/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_H
#define BINREC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In this header (and the library source code in general), "guest" refers
 * to the source CPU or architecture, i.e. the input to the translator, and
 * "host" or "native" refers to the target CPU or architecture, i.e. the
 * output of the translator.  A program which calls into code generated by
 * this library is referred to as a "client program".
 *
 * The numeric values of constants and the layout of structures in this
 * header are _not_ part of the public API; always use the symbolic names
 * rather than the numeric values or offsets when writing code which
 * interfaces to the library.  The library ABI (including those values and
 * offsets) will be kept consistent through revisions of a major version,
 * such that a program compiled against library version x.y (x >= 1) will
 * also run correctly using library version x.z (z > y), but the ABI may
 * change between major versions, or at any time before version 1.0.
 */

/*************************************************************************/
/*********************** Data types and constants ************************/
/*************************************************************************/

/*----------------------------- Basic types -----------------------------*/

/**
 * binrec_t:  Type of a translation handle.  This handle stores global
 * translation settings, such as optimization flags and functions to use
 * for memory allocation.
 */
typedef struct binrec_t binrec_t;

/**
 * binrec_arch_t:  Enumeration of architectures and variants supported by
 * the library.  All currently supported architectures are either
 * guest-only or host-only; see the inline comments at each enumerator.
 *
 * As a general rule, libbinrec assumes that its input is a program
 * designed to run on the selected guest architecture, and therefore all
 * instructions encountered in the program will be valid instruction
 * encodings.  Consequently, this enumeration only includes coarse
 * architecture families which encompass a group of compatible processors;
 * for example, the PPC_7XX architecture covers all PowerPC CPUs through
 * the 750CL, and the input program is assumed to use only instructions
 * which are valid on the architecture it was written for. See also the
 * note on library limitations in the README file.
 */
typedef enum binrec_arch_t {
    /* Constant used to indicate an unsupported architecture by
     * binrec_native_arch(). */
    BINREC_ARCH_INVALID = 0,

    /* PowerPC 32-bit architecture as implemented in PowerPC 7xx
     * processors, including all other instruction set extensions through
     * the PowerPC 750CL.  Also supports programs written for PowerPC 6xx
     * CPUs, with the exception of non-PowerPC instructions (such as ABS)
     * specific to the PowerPC 601. */
    BINREC_ARCH_PPC_7XX,                // Guest only.

    /* Intel/AMD x86 64-bit architecture, using the SysV ABI. */
    BINREC_ARCH_X86_64_SYSV,            // Host only.

    /* Intel/AMD x86 64-bit architecture, using the Windows ABI. */
    BINREC_ARCH_X86_64_WINDOWS,         // Host only.

    /* Variant of BINREC_ARCH_X86_64_WINDOWS which prepends unwind
     * information to the returned function.  The offset to the generated
     * code is stored as a 64-bit value at the returned code address, and
     * the unwind information is found immediately after that value. */
    BINREC_ARCH_X86_64_WINDOWS_SEH,     // Host only.
} binrec_arch_t;

/**
 * binrec_loglevel_t:  Enumeration of log levels which can be passed to
 * the log function specified in binrec_setup_t.
 */
typedef enum binrec_loglevel_t {
    BINREC_LOGLEVEL_INFO,    // Informational messages.
    BINREC_LOGLEVEL_WARNING, // Messages indicating a potential problem.
    BINREC_LOGLEVEL_ERROR,   // Messages indicating failure of some operation.
} binrec_loglevel_t;

/*--------------------- Architecture feature flags ----------------------*/

/*
 * These flags indicate the presence of specific features (such as optional
 * instructions) within a particular architecture.  These are used in the
 * "host_features" field of binrec_setup_t.
 */

/**
 * BINREC_FEATURE_X86_*:  Feature flags for the x86 architecture.
 */
#define BINREC_FEATURE_X86_FMA      (1U << 0)
#define BINREC_FEATURE_X86_MOVBE    (1U << 1)
#define BINREC_FEATURE_X86_POPCNT   (1U << 2)
#define BINREC_FEATURE_X86_AVX      (1U << 3)
#define BINREC_FEATURE_X86_LZCNT    (1U << 4)  // a.k.a. ABM
#define BINREC_FEATURE_X86_BMI1     (1U << 5)
#define BINREC_FEATURE_X86_AVX2     (1U << 6)
#define BINREC_FEATURE_X86_BMI2     (1U << 7)

/*--------------------------- Setup structure ---------------------------*/

/**
 * binrec_setup_t:  Structure which defines various parameters used by the
 * translator.  Used by binrec_create_handle().
 */
typedef struct binrec_setup_t {

    /**
     * guest, host:  BINREC_ARCH_* values indicating the architecture and
     * variant to translate from (guest) and to (host).  binrec_translate()
     * will fail if the library cannot perform the requested translation.
     */
    binrec_arch_t guest;
    binrec_arch_t host;

    /**
     * host_features:  Bitwise-OR of feature flags for the selected host
     * architecture, indicating which features should be assumed to be
     * present for host code generation.
     */
    unsigned int host_features;

    /**
     * memory_base:  Pointer to a region of host memory reserved as the
     * address space of the guest code.  binrec_translate() calls will read
     * source machine instructions from this region, and generated native
     * machine code will perform loads and stores by adding the target
     * address to this pointer.  Client code is responsible for mapping the
     * parts of this region corresponding to valid guest memory and for
     * handling any invalid-access exceptions which occur during execution
     * (due to code reading from an invalid address).
     */
    void *memory_base;

    /**
     * state_offset_*:  Offsets from the beginning of the processor state
     * block (as passed to the generated native code) to the various guest
     * registers and other processor state.  Each block of registers is
     * assumed to be contiguous; for example, GPR 1 is accessed by loading
     * a 32-bit value from state_offset_gpr + 4.  All multi-byte values are
     * assumed to be stored in host endian order.
     * FIXME: Generalize for non-PPC guests.
     */
    /* General-purpose registers (32 * uint32_t) */
    int state_offset_gpr;
    /* Floating-point registers (32 * double[2]) */
    int state_offset_fpr;
    /* Paired-single quantization registers (8 * uint32_t) */
    int state_offset_gqr;
    /* Miscellaneous registers (each uint32_t) */
    int state_offset_lr;
    int state_offset_ctr;
    int state_offset_cr;
    int state_offset_xer;
    int state_offset_fpscr;
    /* lwarx/stwcx reservation state flag (uint8_t) */
    int state_offset_reserve_flag;
    /* lwarx/stwcx reservation address (uint32_t) */
    int state_offset_reserve_address;
    /* Next instruction address (updated on return from translated code) */
    int state_offset_nia;

    /**
     * userdata:  Opaque pointer which is passed to all callback functions.
     */
    void *userdata;

    /**
     * malloc:  Pointer to a function which allocates memory, like malloc().
     * If NULL, the system's malloc() will be used.
     *
     * Like standard malloc(), this function may return either NULL or a
     * pointer to a zero-size memory block if passed a size of zero.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     size: Size of block to allocate, in bytes.
     * [Return value]
     *     Pointer to allocated memory, or NULL on error or if size == 0.
     */
    void *(*malloc)(void *userdata, size_t size);

    /**
     * realloc:  Pointer to a function which resizes a block of allocated
     * memory, like realloc().  If NULL, the system's realloc() will be used.
     *
     * Like standard realloc(), this function may return either NULL or a
     * pointer to a zero-size memory block if passed a size of zero.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     ptr: Block to resize, or NULL to allocate a new block.
     *     size: New size of block, in bytes, or 0 to free the block.
     * [Return value]
     *     Pointer to allocated memory, or NULL on error or if size == 0.
     */
    void *(*realloc)(void *userdata, void *ptr, size_t size);

    /**
     * free:  Pointer to a function which frees a block of allocated
     * memory, like free().  If NULL, the system's free() will be used.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     ptr: Block to free (may be NULL).
     * [Return value]
     *     Pointer to allocated memory, or NULL on error.
     */
    void (*free)(void *userdata, void *ptr);

    /**
     * code_malloc:  Pointer to a function which allocates a block of
     * memory for output machine code.  If NULL, the malloc() callback (or
     * the system's malloc(), if that callback is also NULL) will be used
     * and no alignment will be performed.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     size: Size of block to allocate, in bytes (guaranteed to be
     *         nonzero).
     *     alignment: Desired address alignment, in bytes (guaranteed to
     *         be a power of 2).
     * [Return value]
     *     Pointer to allocated memory, or NULL on error.
     */
    void *(*code_malloc)(void *userdata, size_t size, size_t alignment);

    /**
     * code_realloc:  Pointer to a function which resizes a block of memory
     * allocated with the code_malloc() callback.  If NULL, the realloc()
     * callback (or the system's realloc(), if that callback is also NULL)
     * will be used.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     ptr: Block to resize (guaranteed to be non-NULL).
     *     old_size: Current size of block, in bytes.
     *     new_size: New size of block, in bytes (guaranteed to be nonzero).
     *     alignment: Required address alignment, in bytes (guaranteed to
     *         be equal to the value used for initial allocation).
     * [Return value]
     *     Pointer to allocated memory, or NULL on error.
     */
    void *(*code_realloc)(void *userdata, void *ptr, size_t old_size,
                          size_t new_size, size_t alignment);

    /**
     * code_free:  Pointer to a function which frees a block of memory
     * allocated with the code_malloc() callback.  If NULL, the free()
     * callback (or the system's free(), if that callback is also NULL)
     * will be used.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     ptr: Block to free (may be NULL).
     */
    void (*code_free)(void *userdata, void *ptr);

    /**
     * log:  Pointer to a function to log messages from the library.
     * If NULL, no logging will be performed.
     *
     * [Parameters]
     *     userdata: User data pointer from setup structure.
     *     level: Log level (BINREC_LOGLEVEL_*).
     *     message: Log message.
     */
    void (*log)(void *userdata, binrec_loglevel_t level, const char *message);

} binrec_setup_t;

/*--------------------- General optimization flags ----------------------*/

/*
 * Optimizations performed by the library can generally be classified into
 * three types:
 *
 * - Behavior-safe: optimizations which purely affect the size or speed of
 *   the generated code and have no effect on behavior.  Optimizations
 *   such as constant folding and deconditioning fall into this category.
 *
 * - Specification-safe: optimizations which may change the behavior of
 *   the generated code, but only within limits prescribed by the relevant
 *   specification.  For example, the NATIVE_IEEE_TINY optimization may
 *   change the results of certain floating-point operations relative to
 *   the results returned by guest code running on its native hardware,
 *   but the IEEE floating-point specification allows either of two
 *   behaviors, so with respect to that specification, the optimizaed code
 *   is no less correct than the original.  As long as the guest code was
 *   written to follow the specifications rather than the precise behavior
 *   of the guest hardware, it will still behave correctly under these
 *   optimizations.
 *
 * - Unsafe: optimizations which can materially impact the behavior of the
 *   generated code, such as stack frame optimization.  These optimizations
 *   can benefit code which rigorously adhere to the relevant assumptions,
 *   such as code produced by a high-level language compiler, but they can
 *   cause nonconformant code to misbehave or even crash.  Constants for
 *   these optimizations have an "_UNSAFE" suffix appended to indicate this
 *   fact.
 */

/**
 * BINREC_OPT_BASIC:  Enable basic optimization of translated code.  This
 * includes the following transformations:
 *
 * - Load instructions which reference memory areas marked read-only
 *   (see binrec_add_readonly_region()) are replaced by load-immediate
 *   instructions using the value read from memory.
 *
 * - Dead stores (assignments to registers which are never referenced)
 *   are eliminated from the translated code.
 *
 * - Branches to other (unconditional or same-conditioned) branch
 *   instructions will be threaded through to the final branch destination.
 */
#define BINREC_OPT_BASIC  (1<<0)

/**
 * BINREC_OPT_CALLEE_SAVED_REGS_UNSAFE:  Assume that registers specified
 * as callee-saved by the guest ABI are not modified across a subroutine
 * call or system call instruction, and if the instruction can be
 * translated to a native subroutine call, do not flush those registers to
 * the processor state block before the call or reload them after the call.
 *
 * This optimization has no effect unless BINREC_OPT_NATIVE_CALLS is also
 * enabled.  This optimization is also irrelevant for inlined functions;
 * register live ranges will be correctly calculated for all registers
 * when a function is inlined.
 *
 * This optimization is UNSAFE: if the assumption described above is
 * violated by guest code, the translated code will not behave correctly.
 */
#define BINREC_OPT_CALLEE_SAVED_REGS_UNSAFE  (1<<1)

/**
 * BINREC_OPT_DECONDITION:  Convert conditional branches and moves with
 * constant conditions to unconditional instructions or NOPs.  This is
 * most useful in conjunction with constant folding.
 */
#define BINREC_OPT_DECONDITION  (1<<2)

/**
 * BINREC_OPT_DROP_DEAD_BLOCKS:  Eliminate unreachable basic blocks from
 * the code stream.  This is most useful in conjunction with deconditioning.
 */
#define BINREC_OPT_DROP_DEAD_BLOCKS  (1<<3)

/**
 * BINREC_OPT_DROP_DEAD_BRANCHES:  Eliminate branches to the following
 * instruction.
 */
#define BINREC_OPT_DROP_DEAD_BRANCHES  (1<<4)

/**
 * BINREC_OPT_FOLD_CONSTANTS:  Look for computations whose operands are all
 * constant, and convert them to load-immediate operations.  The computed
 * values are themselves treated as constant, so constantness can be
 * propagated through multiple instructions; intermediate values in a
 * computation sequence which end up being unused due to constant folding
 * will be removed via dead store elimination.
 */
#define BINREC_OPT_FOLD_CONSTANTS  (1<<5)

/**
 * BINREC_OPT_NATIVE_CALLS:  Treat subroutine-call instructions (like x86
 * CALL or PowerPC BL) as instructions with side effects rather than
 * branches, by translating them into native subroutine-call instructions.
 *
 * This optimization can significantly improve the performance of non-leaf
 * functions by allowing larger parts of the function to be translated as
 * a single unit.
 *
 * This optimization is behavior-safe in the narrow sense that it does not
 * change the meaning of the code, but code which uses call instructions in
 * nonstandard ways (such as a call to the next instruction to obtain the
 * instruction's address) can potentially cause a host stack overflow if
 * executed too often without returning control to the client program.
 */
#define BINREC_OPT_NATIVE_CALLS  (1<<6)

/**
 * BINREC_OPT_NATIVE_IEEE_TINY:  Use the host's definition of "tiny" for
 * IEEE floating-point arithmetic, even when that differs from the guest
 * definition.
 *
 * When translating between architectures which use different definitions
 * of "tiny" (IEEE allows two different behaviors: tiny before rounding
 * and tiny after rounding), this optimization allows floating-point
 * operations to be translated directly to their equivalent host
 * instructions, at the cost of slightly different results for operations
 * with a result which is treated as "tiny" on one architecture and not
 * the other.  If this optimization is disabled, floating-point operations
 * must check explicitly for a "tiny" result, which can require several
 * additional host instructions per guest instruction.
 *
 * If the host and guest use the same "tiny" rules, floating-point
 * operations can always be translated directly to native instructions
 * (at least with regard to tininess), and this flag has no effect on
 * translation.
 */
#define BINREC_OPT_NATIVE_IEEE_TINY  (1<<7)

/**
 * BINREC_OPT_STACK_FRAMES_UNSAFE:  Assume that a given function's stack
 * frame is only stored to by that function (unless it passes a stack-based
 * pointer to another function), and translate store/load instruction pairs
 * on the same stack frame location (such as when saving and restoring
 * callee-saved registers, or loading a spilled temporary register) to a
 * register move or no-op.
 *
 * If the function passes a stack-based pointer to another function
 * (specifically, if a value derived from the stack pointer register is
 * live in a register or stored to a function argument slot on the stack),
 * this optimization will be disabled for that function.  This optimization
 * only takes effect when both store and load are in the same translation
 * unit, so it will have limited benefit unless BINREC_OPT_NATIVE_CALLS is
 * also enabled.
 *
 * This optimization is UNSAFE: if the assumption described above is
 * violated by guest code, the translated code will not behave correctly.
 */
#define BINREC_OPT_STACK_FRAMES_UNSAFE  (1<<8)

/*----------- Guest-architecture-specific optimization flags ------------*/

/**
 * BINREC_OPT_G_PPC_CONSTANT_GQRS_UNSAFE:  Assume that the values of the
 * GQRs (graphics quantization registers, used with paired-single load and
 * store instructions) are constant with respect to the entry point of a
 * translation unit.
 *
 * If this optimization is enabled, the translator will read the values of
 * any GQRs referenced by guest code and translate paired-single load and
 * store instructions to appropriate host instructions based on those
 * values.  Otherwise, the translated host code will read the GQRs and
 * choose an appropriate load or store operation at runtime, which is
 * typically more than an order of magnitude slower.
 *
 * If guest code modifies a GQR and then performs a paired-single load or
 * store based on that GQR, the translated code will take the new value of
 * the GQR into account; if the value written to the GQR is not a constant,
 * this optimization will be effectively disabled for the remainder of the
 * translation unit.
 *
 * This optimization is UNSAFE: if the assumption described above is
 * violated by guest code, the translated code will not behave correctly.
 */
#define BINREC_OPT_G_PPC_CONSTANT_GQRS_UNSAFE  (1<<0)

/**
 * BINREC_OPT_G_PPC_NATIVE_RECIPROCAL:  Translate guest PowerPC
 * reciprocal-estimate instructions (fres and frsqrte) directly to their
 * host equivalents, maintaining compliance with the PowerPC architecture
 * specification but disregarding the precise behavior of the guest
 * architecture.
 *
 * The PowerPC architecture specifies bounds within which the results of
 * these instructions will fall relative to the true (mathematical) result.
 * Programs written to be compliant with the architecture will work
 * correctly regardless of the exact output of the instruction, though the
 * precise behavior of the program (for example, the low-end bits of the
 * result) may change.  This flag allows the translator to choose faster
 * host instructions which may not give exactly the same result but still
 * satisfy the PowerPC architecture constraints.
 *
 * If this optimization is disabled, the translator will attempt to match
 * the precise behavior of the guest architecture, at the cost of several
 * additional host instructions per affected guest instruction.
 */
#define BINREC_OPT_G_PPC_NATIVE_RECIPROCAL  (1<<1)

/*------------ Host-architecture-specific optimization flags ------------*/

/**
 * BINREC_OPT_H_X86_CONDITION_CODES:  Track the state of the condition
 * codes in the EFLAGS register, and avoid adding an explicit TEST
 * instruction for a register if the condition codes already reflect the
 * value of that register.
 */
#define BINREC_OPT_H_X86_CONDITION_CODES  (1<<0)

/**
 * BINREC_OPT_H_X86_FIXED_REGS:  When an instruction requires an operand to
 * be in a specific hardware register (shift counts must be in CL, for
 * example), try harder to allocate that hardware register for the operand.
 * This requires an extra pass over the translated machine code during
 * register allocation.
 */
#define BINREC_OPT_H_X86_FIXED_REGS  (1<<1)

/**
 * BINREC_OPT_H_X86_FORWARD_CONDITIONS:  When a register used as the
 * condition for a conditional branch or move is the boolean result of a
 * comparison, forward the comparison condition to the branch or move
 * instruction instead of storing the comparison result in a register and
 * testing the zeroness of that register.
 */
#define BINREC_OPT_H_X86_FORWARD_CONDITIONS  (1<<2)

/**
 * BINREC_OPT_H_X86_MEMORY_OPERANDS:  Make use of register-memory forms of
 * x86 instructions in the following cases:
 *
 * - When the second (non-overwritten) operand to a computational
 *   instruction (ADD, SLT, etc.) is the result of a previous load
 *   instruction in the same basic block, the associated register is not
 *   live past the end of the basic block, and there are no store
 *   instructions between the register's first and last use.
 *
 * - When the destination operand to a computational instruction is only
 *   used as the source for a single subsequent store instruction in the
 *   same basic block and there are no intervening load instructions.
 *
 * If this optimization is disabled, all computational instructions will
 * be performed in registers, and loads and stores will be translated as
 * separate instructions.
 */
#define BINREC_OPT_H_X86_MEMORY_OPERANDS  (1<<3)

/**
 * BINREC_OPT_H_X86_SETCC_ZX:  Detect when the only the low byte of the
 * result of a comparison instruction is used, and suppress zero-extension
 * of the result to the full operand size (32 or 64 bits).
 */
#define BINREC_OPT_H_X86_SETCC_ZX  (1<<4)

/*************************************************************************/
/******** Interface: Library and runtime environment information *********/
/*************************************************************************/

/**
 * binrec_version:  Return the version number of the library as a string
 * (for example, "1.2.3").
 *
 * [Return value]
 *     Library version number.
 */
extern const char *binrec_version(void);

/**
 * binrec_native_arch:  Return a BINREC_ARCH_* constant representing the
 * architecture of the runtime environment, or 0 (BINREC_ARCH_INVALID) if
 * the runtime environment does not correspond to a supported host
 * architecture.  (If a nonzero value is returned, it will always be valid
 * as a host architecture for translation.)
 *
 * [Return value]
 *     Runtime environment architecture (BINREC_ARCH_*), or 0 if unsupported.
 */
extern binrec_arch_t binrec_native_arch(void);

/**
 * binrec_native_features:  Return a bitmask of architecture features
 * (BINREC_FEATURE_*) supported by the runtime environment, or 0 if the
 * runtime environment does not correspond to a supported architecture.
 *
 * [Return value]
 *     Runtime environment feature bitmap, or 0 if unsupported.
 */
extern unsigned int binrec_native_features(void);

/*************************************************************************/
/*************** Interface: Translation handle management ****************/
/*************************************************************************/

/**
 * binrec_create_handle:  Create a new translation handle.
 *
 * [Parameters]
 *     setup: Pointer to a binrec_setup_t structure that defines the
 *         translation parameters to use.
 * [Return value]
 *     Newly created handle, or NULL on error.
 */
extern binrec_t *binrec_create_handle(const binrec_setup_t *setup);

/**
 * binrec_destroy_handle:  Destroy a translation handle.
 *
 * This function only destroys the translation handle itself; blocks of
 * translated code generated through the handle remain valid even after
 * the handle is destroyed.
 *
 * [Parameters]
 *     handle: Handle to destroy (may be NULL).
 */
extern void binrec_destroy_handle(binrec_t *handle);

/**
 * binrec_set_code_range:  Set the minimum and maximum addresses from which
 * to read source machine instructions.  Branch instructions which attempt
 * to jump outside this range will terminate the translation unit, and if
 * the source machine code runs off the end of the range, the unit will be
 * terminated at the final instruction completely contained within the range.
 * The range must not wrap around the end of the address space.
 *
 * By default, the entire address space is considered valid for reading
 * instructions.
 *
 * This range can be changed for each binrec_translate() call to limit the
 * length of the guest code routine to translate in that call, though
 * binrec_translate() will end translation on its own when it finds a
 * reasonable stopping point (such as a return-from-subroutine instruction).
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     start: First address of code range.
 *     end: Last address of code range (inclusive).
 */
extern void binrec_set_code_range(binrec_t *handle, uint32_t start,
                                  uint32_t end);

/**
 * binrec_set_optimization_flags:  Set which optimizations should be
 * performed on translated blocks.  Enabling more optimizations will
 * improve the performance of translated code but increase the overhead
 * of translation; see the documentation on each optimization flag for
 * details.
 *
 * The set of enabled optimizations may be changed at any time without
 * impacting the behavior of already-translated blocks.
 *
 * By default, no optimizations are enabled.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     common_opt: Bitmask of common optimizations to apply (BINREC_OPT_*).
 *     guest_opt: Bitmask of guest-specific optimizations to apply
 *         (BINREC_OPT_G_*).
 *     host_opt: Bitmask of host-specific optimizations to apply
 *         (BINREC_OPT_H_*).
 */
extern void binrec_set_optimization_flags(
    binrec_t *handle, unsigned int common_opt, unsigned int guest_opt,
    unsigned int host_opt);

/**
 * binrec_set_max_inline_length:  Set the maximum length (number of source
 * instructions, including the final return instruction) of subroutines to
 * inline.  The default is zero, meaning no subroutines will be inlined.
 *
 * If a nonzero length limit is set with this function, then when the
 * translator encounters a subroutine call instruction to a fixed address,
 * it will scan ahead up to this many instructions for a return
 * instruction.  If one is found, and if there are no branch instructions
 * that branch past the return, the subroutine will be inlined into the
 * current translation unit, saving the cost of jumping to a different
 * unit (which can be significant depending on how many guest registers
 * need to be spilled).
 *
 * If an inlined subroutine contains a further call instruction, that
 * subroutine will not be inlined regardless of its length.  (But see
 * binrec_set_max_inline_depth() to enable such recursive inlining.)
 *
 * Note that if a nonzero length limit is set, inlining may be performed
 * regardless of whether any optimization flags are set.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     length: Maximum inline length (must be at least 0).
 */
extern void binrec_set_max_inline_length(binrec_t *handle, int length);

/**
 * binrec_set_max_inline_depth:  Set the maximum depth of subroutines to
 * inline.  The default is 1.
 *
 * If a depth limit greater than 1 is set with this function, then when a
 * call instruction is encountered during inlining, the translator will
 * perform the same inlining check on the called subroutine, up to the
 * specified depth.  For example, when translating at A in the following
 * pseudocode:
 *     A: call B
 *        ret
 *     B: call C
 *        ret
 *     C: call D
 *        ret
 *     D: call E
 *        ret
 *     E: nop
 *        ret
 * if the maximum inline depth is set to 2 (and assuming the maximum length
 * is set to at least 2), both B and C will be inlined, but D will not, and
 * the A routine will be translated as if it was written:
 *     A: call D
 *        ret
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     depth: Maximum inline depth (must be at least 1).
 */
extern void binrec_set_max_inline_depth(binrec_t *handle, int depth);

/**
 * binrec_add_readonly_region:  Mark the given region of memory as
 * read-only.  Instructions which are known to load from read-only memory
 * will be translated into load-constant operations if optimizations are
 * enabled (with the BINREC_OPT_ENABLE flag).
 *
 * This function may fail if too many misaligned regions are added; in
 * that case, rebuild the library with different values of the
 * READONLY_PAGE_BITS and MAX_PARTIAL_READONLY constants in src/common.h.
 *
 * The address range specified by binrec_set_code_range() is treated as
 * read-only with respect to the instruction stream, regardless of whether
 * it is explicitly marked read-only via this function.  However, any data
 * interspersed within the instruction stream will only be treated as
 * constant data if the address of that data has been explicitly marked
 * read-only.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     base: Base address (in guest memory) of read-only region.
 *     size: Size of read-only region, in bytes.
 * [Return value]
 *     True on success; false if the region could not be added because
 *     the partial-page table is full.
 */
extern int binrec_add_readonly_region(binrec_t *handle,
                                      uint32_t base, uint32_t size);

/**
 * binrec_clear_readonly_regions:  Clear all read-only memory regions
 * added with binrec_add_readonly_region().
 *
 * [Parameters]
 *     handle: Handle to operate on.
 */
extern void binrec_clear_readonly_regions(binrec_t *handle);

/*************************************************************************/
/********************** Interface: Code translation **********************/
/*************************************************************************/

/**
 * binrec_translate:  Translate a block of guest machine code into native
 * machine code.
 *
 * On success, the returned block can be executed by calling it as a
 * function with the following signature:
 *     void *code(void *state);
 * where the single parameter is a pointer to a processor state block whose
 * structure conforms to the structure offsets specified in the setup data
 * passed to binrec_create_handle(), and the return value is the processor
 * state block in use after execution of the code (this may differ from the
 * passed-in pointer if, for example, the code is running in a simulated
 * multi-processor environment and a system call instruction causes the
 * code to be rescheduled on a different simulated processor).
 *
 * The returned code pointer will have been allocated with the code_malloc
 * or code_realloc function passed in the setup structure to
 * binrec_create_handle(), or the relevant fallback function if code_*
 * functions were not supplied.
 *
 * Return-value arguments are only modified on a successful return.
 *
 * [Parameters]
 *     handle: Handle to use for translation.
 *     address: Address (in guest memory) of first instruction to translate.
 *     code_ret: Pointer to variable to receive a pointer to the
 *         translated machine code.
 *     size_ret: Pointer to variable to receive the length of the
 *         translated machine code, in bytes.
 * [Return value]
 *     True (nonzero) on success, false (zero) on error.
 */
extern int binrec_translate(binrec_t *handle, uint32_t address,
                            void **code_ret, long *size_ret);

/*************************************************************************/
/*************************************************************************/

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // BINREC_H
