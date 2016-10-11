/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_HOST_X86_INTERNAL_H
#define BINREC_HOST_X86_INTERNAL_H

#include "src/common.h"
#include "src/rtl.h"

/*
 * Notes on terminology and typography:
 *
 * - When not qualified with a specific bit width, "word" refers to a
 *   32-bit unit.  This differs from Intel usage, but follows the usage of
 *   all other major architectures currently in use.
 *
 * - In comments, register names such as "rAX" with a lower-case "r"
 *   indicate an indeterminate data size, while "RAX" and "EAX"
 *   indicate that the register is being used in 64-bit or 32-bit mode
 *   respectively.  In some cases (notably the register constants defined
 *   below), the prefix may be omitted for brevity; this is considered
 *   unlikely to cause confusion since 16-bit registers are only used in
 *   a few locations.
 *
 * - Assembly instructions in comments (particularly in tests) follow
 *   AT&T syntax, as seen in GCC-style inline assembly: for example,
 *   "movl $0x1234,(%rsp)" rather than "MOV DWORD PTR [RSP],1234h".
 */

/*************************************************************************/
/**************** Internal constants and data structures *****************/
/*************************************************************************/

/* Constants identifying x86 registers.  These are defined such that the
 * low 3 bits of the each value match the register's index in various
 * opcode bytes. */
typedef enum X86Register {
    X86_AX    = 0,
    X86_CX    = 1,
    X86_DX    = 2,
    X86_BX    = 3,
    X86_SP    = 4,
    X86_BP    = 5,
    X86_SI    = 6,
    X86_DI    = 7,
    X86_R8    = 8,
    X86_R9    = 9,
    X86_R10   = 10,
    X86_R11   = 11,
    X86_R12   = 12,
    X86_R13   = 13,
    X86_R14   = 14,
    X86_R15   = 15,
    X86_XMM0  = 16,
    X86_XMM1  = 17,
    X86_XMM2  = 18,
    X86_XMM3  = 19,
    X86_XMM4  = 20,
    X86_XMM5  = 21,
    X86_XMM6  = 22,
    X86_XMM7  = 23,
    X86_XMM8  = 24,
    X86_XMM9  = 25,
    X86_XMM10 = 26,
    X86_XMM11 = 27,
    X86_XMM12 = 28,
    X86_XMM13 = 29,
    X86_XMM14 = 30,
    X86_XMM15 = 31,
} X86Register;

/* Constants for the 4 variable bits in a REX prefix. */
enum {
    X86_REX_W = 8,
    X86_REX_R = 4,
    X86_REX_X = 2,
    X86_REX_B = 1,
};

/*-----------------------------------------------------------------------*/

/* Data associated with each RTL register. */
typedef struct HostX86RegInfo {
    /* Has a host register been allocated for this register? */
    bool host_allocated;
    /* Index of allocated host register (X86Register). */
    uint8_t host_reg;

    /* Has a host register been allocated for temporary values in the
     * instruction that sets this register? */
    bool temp_allocated;
    /* Host register (X86Register) to use for temporary values. */
    uint8_t host_temp;

    /* Bitmask of host registers not to allocate for this register. */
    uint32_t avoid_regs;

    /* Has a spill location been allocated for this register? */
    bool spilled;
    /* Byte offset from SP of the spill storage location. */
    int16_t spill_offset;
    /* Instruction index at which the register was spilled. */
    int32_t spill_insn;

    /* Should predecessor blocks load this register with its alias's value
     * on block exit? (see RTLRegister.alias for the alias number) */
    bool merge_alias;
    /* Target host register for alias loads.  This might not be the same
     * as host_reg! */
    uint8_t host_merge;

    /* Next register in the fixed-allocation list, or 0 if the end of the
     * list. */
    uint16_t next_fixed;
} HostX86RegInfo;

/* Data associated with each basic block. */
typedef struct HostX86BlockInfo {
    /* Bitmap of host registers which are live at the end of the block. */
    uint32_t end_live;

    /* Array of RTL registers into which each alias register is first
     * loaded in the block, or 0 if the alias is not read. */
    uint16_t *alias_load;
    /* Array of RTL registers stored to each alias register at the end of
     * the block, or 0 if the alias is not written. */
    uint16_t *alias_store;

    /* Code buffer offset of the 32-bit displacement operand to an
     * unresolved branch instruction at the end of this block, or -1 if
     * the block does not have an unresolved branch. */
    long unresolved_branch_offset;
    /* Label targeted by the unresolved branch, or 0 if it targets the
     * function epilogue. */
    int unresolved_branch_target;
} HostX86BlockInfo;

/* Context block used to maintain translation state. */
typedef struct HostX86Context {
    /* Arguments passed from binrec_translate(). */
    binrec_t *handle;
    RTLUnit *unit;

    /* Bitmap of callee-saved registers for the selected ABI. */
    uint32_t callee_saved_regs;

    /* Information for each basic block. */
    HostX86BlockInfo *blocks;
    /* Information for each RTL register. */
    HostX86RegInfo *regs;
    /* Generated code offsets for each label, and for the epilogue (which
     * is handled internally as label 0).  -1 indicates the label has not
     * yet been seen. */
    long *label_offsets;
    /* Buffer for alias_* arrays for each block. */
    void *alias_buffer;

    /* Current mapping from x86 to RTL registers. */
    uint16_t reg_map[32];
    /* Current bitmap of available registers. */
    uint32_t regs_free;
    /* Bitmap of registers which are used at least once (for saving and
     * restoring registers in the prologue/epilogue). */
    uint32_t regs_touched;
    /* Bitmap of registers which have been used in the current block. */
    uint32_t block_regs_touched;

    /* First CALL instruction which is not a tail call, or -1 if there is
     * no such instruction.  Subsequent non-tail calls are linked via the
     * RTLInsn.host_data_32 field (only valid during register allocation).
     * After register allocation, tailness is recorded during register
     * allocation as a boolean in the host_data_16 field of each CALL
     * instruction; for non-tail calls, the set of registers to be saved
     * and restored is recorded in the host_data_32 field, replacing the
     * link for this list. */
    int32_t nontail_call_list;
    int32_t last_nontail_call;  // Last instruction in the list.
    /* List of registers which are allocated to specific hardware registers
     * due to instruction requirements (e.g. shift counts in CL).  The
     * first register in the list is stored here, and subsequent registers
     * (in birth order) can be found by following the next_fixed links. */
    uint16_t fixed_reg_list;
    /* Instruction at which the register currently allocated to rAX/rCX/rDX
     * dies.  Used by the register allocator in fixed-register optimization. */
    int32_t last_ax_death;
    int32_t last_cx_death;
    int32_t last_dx_death;

    /* Stack frame size. */
    int frame_size;
    /* Total stack allocation, excluding PUSH instructions. */
    int stack_alloc;
    /* Stack offset of temporary storage for call-clobbered registers,
     * indexed by X86Register number (-1 if no offset assigned). */
    int stack_callsave[32];
} HostX86Context;

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

/**
 * host_x86_allocate_registers:  Allocate a host register for each RTL
 * register which needs one.
 *
 * On return, ctx->callee_saved_regs will be set to the proper value for
 * the selected ABI.
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     True on success, false on error.
 */
#define host_x86_allocate_registers INTERNAL(host_x86_allocate_registers)
extern bool host_x86_allocate_registers(HostX86Context *ctx);

/**
 * host_x86_int_arg_register:  Return the register used for the given
 * integer function argument.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: Argument index.
 * [Return value]
 *     Register for argument (X86Register), or -1 if the given argument is
 *     not passed in a register.
 */
#define host_x86_int_arg_register INTERNAL(host_x86_int_arg_register)
extern PURE_FUNCTION int host_x86_int_arg_register(
    HostX86Context *ctx, int index);

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_HOST_X86_INTERNAL_H
