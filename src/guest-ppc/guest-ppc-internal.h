/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_GUEST_PPC_INTERNAL_H
#define BINREC_GUEST_PPC_INTERNAL_H

#include "src/common.h"
#include "src/endian.h"
#include "src/rtl.h"

struct RTLInsn;

/*
 * Notes on translator design
 * ==========================
 *
 * Floating-point registers
 * ------------------------
 * Floating-point registers (FPRs) can be used in up to three modes:
 * double-precision, single-precision, and the 750CL-specific paired-single
 * mode.  Correctly(*) handling all three modes requires a fair amount of
 * analysis and state tracking, as described below.
 *
 * (*) The translator does not attempt to reproduce the data hazards
 *     present on the 750CL when using double precision and paired-single
 *     instructions on the same register in close proximity without
 *     intervening format-conversion operations.  Such instruction
 *     sequences are explicitly documented as a "programming error" in the
 *     750CL manual, and it should be reasonable to assume that real-world
 *     programs do not rely on such behavior.
 *
 * During the initial scan, we track whether each FPR is used with any
 * paired-single instructions.  We then allocate an alias for each FPR used
 * in the translation unit, of type V2_FLOAT64 if the register is ever used
 * in paired-single mode and FLOAT64 (scalar) if not, bound to the
 * appropriate location in the processor state block.
 *
 * During actual translation, the translator tracks the current data type
 * stored in the register, to avoid converting back to the alias type on
 * every operation.  When a format conversion is needed, it is performed
 * according to the following table:
 *
 *    V2_FLOAT64 -> FLOAT32: extract element 0, convert to FLOAT32
 *    V2_FLOAT64 -> FLOAT64: extract element 0
 *    V2_FLOAT64 -> V2_FLOAT32: convert to V2_FLOAT32
 *
 *    V2_FLOAT32 -> FLOAT32: extract element 0
 *    V2_FLOAT32 -> FLOAT64: convert to V2_FLOAT64, copy to alias,
 *                              extract element 0
 *    V2_FLOAT32 -> V2_FLOAT64: convert to V2_FLOAT64
 *
 *    FLOAT64 -> FLOAT32: convert to FLOAT32
 *    FLOAT64 -> V2_FLOAT32: load alias, insert into element 0, convert
 *                              result to V2_FLOAT32
 *    FLOAT64 -> V2_FLOAT64: load alias, insert into element 0
 *
 *    FLOAT32 -> FLOAT64: convert to FLOAT64; if alias is not of type
 *                           V2_FLOAT64, store to second slot in PSB
 *    FLOAT32 -> V2_FLOAT32: broadcast to V2_FLOAT32
 *    FLOAT32 -> V2_FLOAT64: convert to FLOAT64, broadcast to V2_FLOAT64
 *
 * Note the logic for converting from FLOAT32; all instructions with
 * scalar single-precision outputs are architecturally defined to copy
 * their result to the second paired-single slot (except frsp, but in fact
 * that instruction is implemented to copy its result in the same manner,
 * so we don't have to treat it specially).
 *
 * CR and FPSCR bits
 * -----------------
 * The condition register (CR) is normally accessed only in units of 4
 * bits (a CR field written by a compare or Rc=1 instruction) or 1 bit (a
 * specific bit tested by a conditional branch).  In order to allow the
 * RTL core to detect dead stores, such as multiple compare instructions
 * writing to the same CR field, we treat each bit of CR as a separate
 * "register" with an associated alias; the bit value is extracted from CR
 * into the alias at the beginning of the unit if its value is needed, and
 * all modified bits are merged back into the CR word when returning to
 * the translated code's caller.  This allows the RTL core to detect and
 * eliminate dead stores to individual bits: for example, if a unit
 * contains several pairs of compare and branch-if-equal instructions, the
 * LT, GT, and SO stores (and the associated compare/extract operations)
 * can be omitted as long as there is another compare later in the unit
 * which overwrites them.
 *
 * In some cases, this will not be enough to fully eliminate unnecessary
 * stores.  Consider this (admittedly somewhat contrived) instruction
 * sequence:
 *
 *       andi. r0,r3,8
 *       bne 4f
 *       andi. r0,r3,4
 *       bne 3f
 *       andi. r0,r3,2
 *       bne 2f
 *       andi. r0,r3,1
 *       bne 1f
 *       b 0f
 *    4: addi r4,r4,1
 *    3: addi r4,r4,1
 *    2: addi r4,r4,1
 *    1: addi r4,r4,1
 *    0: blr
 *
 * In this example, whichever branch is taken will leave the unit without
 * further writes to CR, so none of the bit stores can be trivially
 * eliminated, but in each of the first three cases, the CR write is dead
 * if the branch is not taken.  The BINREC_OPT_G_PPC_TRIM_CR_STORES
 * optimization improves performance for code such as this by finding
 * cases in which CR stores are only visible on one side of a conditional
 * branch and moving the bit stores, along with the instructions that set
 * those bits (typically set-conditional or bit-extract operations), so
 * they are only executed on that code path.  While this does not reduce
 * code size (and may actually increase it slightly, since setting bits on
 * the taken side of a conditional branch requires flipping the sense of
 * the branch and adding an extra unconditional jump afterward), it does
 * help ensure that the actual control flow goes through as few
 * CR-modifying instructions as possible.
 *
 * Similar logic applies to the FI, FR, and FPRF fields of FPSCR, which
 * are rewritten by most floating-point instructions.  Since these bits
 * are always written as a unit (except in the case of fcmp/ps_cmp, which
 * only write the FPCC bits, and the mtfs group of instructions) and since
 * there are no instructions which read individual bits from those fields,
 * we allocate a single alias for all 7 bits instead of separate aliases
 * for each bit.  The remainder of the register is not treated specially;
 * exception bits accumulate rather than being overwritten by each
 * instruction, and there are no instructions other than the mtfs group
 * which write RN, so there is no benefit to using separate aliases for
 * each bit.
 */

/*************************************************************************/
/*********************** Architectural constants *************************/
/*************************************************************************/

/* XER register bits. */
#define XER_SO_SHIFT  31
#define XER_OV_SHIFT  30
#define XER_CA_SHIFT  29
#define XER_SO  (1 << XER_SO_SHIFT)
#define XER_OV  (1 << XER_OV_SHIFT)
#define XER_CA  (1 << XER_CA_SHIFT)

/* FPSCR register bits. */
#define FPSCR_FX_SHIFT      31
#define FPSCR_FEX_SHIFT     30
#define FPSCR_VX_SHIFT      29
#define FPSCR_OX_SHIFT      28
#define FPSCR_UX_SHIFT      27
#define FPSCR_ZX_SHIFT      26
#define FPSCR_XX_SHIFT      25
#define FPSCR_VXSNAN_SHIFT  24
#define FPSCR_VXISI_SHIFT   23
#define FPSCR_VXIDI_SHIFT   22
#define FPSCR_VXZDZ_SHIFT   21
#define FPSCR_VXIMZ_SHIFT   20
#define FPSCR_VXVC_SHIFT    19
#define FPSCR_FR_SHIFT      18
#define FPSCR_FI_SHIFT      17
#define FPSCR_FPRF_SHIFT    12
#define FPSCR_RESV20_SHIFT  11  // Reserved bit, can't be set.
#define FPSCR_VXSOFT_SHIFT  10
#define FPSCR_VXSQRT_SHIFT   9
#define FPSCR_VXCVI_SHIFT    8
#define FPSCR_VE_SHIFT       7
#define FPSCR_OE_SHIFT       6
#define FPSCR_UE_SHIFT       5
#define FPSCR_ZE_SHIFT       4
#define FPSCR_XE_SHIFT       3
#define FPSCR_NI_SHIFT       2
#define FPSCR_RN_SHIFT       0
#define FPSCR_FX      (1 << FPSCR_FX_SHIFT)
#define FPSCR_FEX     (1 << FPSCR_FEX_SHIFT)
#define FPSCR_VX      (1 << FPSCR_VX_SHIFT)
#define FPSCR_OX      (1 << FPSCR_OX_SHIFT)
#define FPSCR_UX      (1 << FPSCR_UX_SHIFT)
#define FPSCR_ZX      (1 << FPSCR_ZX_SHIFT)
#define FPSCR_XX      (1 << FPSCR_XX_SHIFT)
#define FPSCR_VXSNAN  (1 << FPSCR_VXSNAN_SHIFT)
#define FPSCR_VXISI   (1 << FPSCR_VXISI_SHIFT)
#define FPSCR_VXIDI   (1 << FPSCR_VXIDI_SHIFT)
#define FPSCR_VXZDZ   (1 << FPSCR_VXZDZ_SHIFT)
#define FPSCR_VXIMZ   (1 << FPSCR_VXIMZ_SHIFT)
#define FPSCR_VXVC    (1 << FPSCR_VXVC_SHIFT)
#define FPSCR_FR      (1 << FPSCR_FR_SHIFT)
#define FPSCR_FI      (1 << FPSCR_FI_SHIFT)
#define FPSCR_FPRF    (31 << FPSCR_FPRF_SHIFT)
#define FPSCR_RESV20  (1 << FPSCR_RESV20_SHIFT)
#define FPSCR_VXSOFT  (1 << FPSCR_VXSOFT_SHIFT)
#define FPSCR_VXSQRT  (1 << FPSCR_VXSQRT_SHIFT)
#define FPSCR_VXCVI   (1 << FPSCR_VXCVI_SHIFT)
#define FPSCR_VE      (1 << FPSCR_VE_SHIFT)
#define FPSCR_OE      (1 << FPSCR_OE_SHIFT)
#define FPSCR_UE      (1 << FPSCR_UE_SHIFT)
#define FPSCR_ZE      (1 << FPSCR_ZE_SHIFT)
#define FPSCR_XE      (1 << FPSCR_XE_SHIFT)
#define FPSCR_NI      (1 << FPSCR_NI_SHIFT)
#define FPSCR_RN      (3 << FPSCR_RN_SHIFT)

/* Mask for all VXFOO exception bits. */
#define FPSCR_ALL_VXFOO  (FPSCR_VXSNAN | FPSCR_VXISI | FPSCR_VXIDI \
                          | FPSCR_VXZDZ | FPSCR_VXIMZ | FPSCR_VXVC \
                          | FPSCR_VXSOFT | FPSCR_VXSQRT | FPSCR_VXCVI)

/* Mask for all non-hardwired exception bits. */
#define FPSCR_ALL_EXCEPTIONS  (FPSCR_OX | FPSCR_UX | FPSCR_ZX | FPSCR_XX \
                               | FPSCR_ALL_VXFOO)

/* Mask for all exception enable bits. */
#define FPSCR_ALL_ENABLES  (FPSCR_VE | FPSCR_OE | FPSCR_UE | FPSCR_ZE \
                            | FPSCR_XE)

/* Rounding modes for FPSCR[RN]. */
#define FPSCR_RN_N    0  // Round to nearest.
#define FPSCR_RN_Z    1  // Round toward zero.
#define FPSCR_RN_P    2  // Round toward plus infinity.
#define FPSCR_RN_M    3  // Round toward minus infinity.

/*
 * SPR numbers.
 *
 * TBL and TBU are the time base registers read by the mftb instruction.
 * These can also be retrieved with mfspr, and indeed the manuals of
 * various 32-bit PowerPC CPUs state that "Some implementations may
 * implement mftb and mfspr identically" (750CL) or even "The 603e ignores
 * the extended opcode differences between mftb and mfspr by ignoring bit
 * 25 of both instructions and treating them both identically" (603e), so
 * we treat the time base registers as ordinary SPRs which are read-only to
 * user-mode programs.
 *
 * GQR0-7, the "graphics quantization registers", are specific to the 750CL
 * processor.  These registers are supervisor-mode registers, so as with
 * other supervisor-mode instructions, we treat them as illegal operations.
 *
 * UGQR0-7, which use the same SPR numbers as GQR0-7 with bit 0x10 (the
 * "supervisor-mode register" bit) cleared, are not documented in the
 * PowerPC 750CL manual, but at least some implementations[1] treat them as
 * user-mode equivalents to GQR0-7, allowing user-mode programs to access
 * those registers.  It is not known whether a stock 750CL supports these
 * registers, but we assume that a program which accesses the UGQRs is
 * intended to run on an implementation of the architecture which supports
 * them, so we don't use a subarchitecture identifier or feature flag to
 * explicitly enable them.
 *
 * [1] Notably the "Broadway" and "Espresso" processors used in the
 *     Nintendo Wii and Wii U game systems, respectively.  The GQRs (though
 *     possibly not the UGQRs) are also present in the "Gekko" processor, a
 *     customized 750CXe, used in the earlier Nintendo GameCube game system.
 */
#define SPR_XER     1
#define SPR_LR      8
#define SPR_CTR     9
#define SPR_TBL     268
#define SPR_TBU     269
#define SPR_UGQR(n) (896 + (n))
#define SPR_GQR(n)  (912 + (n))

/*************************************************************************/
/*********************** Internal data structures ************************/
/*************************************************************************/

/* Bitmasks indicating register sets.  Used for data flow analysis. */
typedef struct GuestPPCRegSet {
    uint32_t gpr;
    uint32_t fpr;
    uint32_t crb;
    uint8_t
        cr : 1,  // For instructions which access CR as a whole (mfcr/mtcr).
        lr : 1,
        ctr : 1,
        xer : 1,
        xer_so : 1,
        fpscr : 1,
        fr_fi_fprf : 1;
} GuestPPCRegSet;

/* Scan data for a single basic block. */
typedef struct GuestPPCBlockInfo {
    /* Start address and length (in bytes) of the block. */
    uint32_t start;
    uint32_t len;

    /* Indices of successor blocks, used by the TRIM_CR_STORES optimization.
     * A conditional branch is treated as terminal if either the branch
     * target or the fall-through address lie outside the unit. */
    int next_block;  // -1 if block is terminal or ends in an uncond'l branch.
    int branch_block;  // -1 if block is terminal.

    /* Set of registers which are referenced (read or written) by the block. */
    GuestPPCRegSet touched;
    /* Set of FPR registers used or changed in paired-single mode. */
    uint32_t fpr_is_ps;
    /* Set of CR bits whose values on entry to the block are visible to an
     * instruction within the block. */
    uint32_t crb_used;
    /* Set of CR bits which are written by the block. */
    uint32_t crb_changed;
    /* Flag indicating whether CR as a whole is read by the block. */
    bool cr_used;
    /* Flag indicating whether FPSCR is written by the block. */
    bool fpscr_changed;

    /* Bitmask of CR bits which are changed without being read on every
     * code path out of the unit.  Used by the TRIM_CR_STORES optimization. */
    uint32_t crb_changed_recursive;

    /* Addresses of a paired lwarx and stwcx. in this block (see notes at
     * translate_lwarx() for details), or ~0 if none. */
    uint32_t paired_lwarx;
    uint32_t paired_stwcx;

    /* Does this block contain a trap instruction (tw/twi)? */
    bool has_trap;
    /* Does this block end in a branch of any sort? */
    bool has_branch;
    /* Does this block end in a conditional branch? */
    bool is_conditional_branch;
    /* Is this block a branch target?  (Labels are only allocated for
     * branch targets.) */
    bool is_branch_target;

    /* RTL label for this block, or 0 if none has been allocated. */
    int label;
} GuestPPCBlockInfo;

/*
 * RTL register set corresponding to guest CPU state.
 *
 * FPR aliases in GuestPPCContext.alias.fpr[] are of type FLOAT64 or
 * V2_FLOAT64; depending on how the FPRs are used, additional aliases are
 * allocated in alias_fpr_32[], alias_fpr_64[] (only used if the primary
 * alias is of type V2_FLOAT64), and alias_fpr_ps[].  The register stored
 * in GuestPPCContext.live.fpr[] always holds the current value of the FPR,
 * which may be of any floating-point scalar or vector type.
 *
 * CR is recorded as both 32 1-bit aliases (crb[]) and one 32-bit alias
 * (cr); the semantics of their interaction are that (1) the base value of
 * CR is the value of the RTL alias GuestPPCContext.alias.cr, and (2) for
 * each bit b in CR, if the corresponding bit is set in
 * GuestPPCContext.crb_loaded, the value of the bit is the value of the RTL
 * alias GuestPPCContext.crb[b], else the value of the bit is the value of
 * the corresponding bit in the base value of CR as determined above.
 *
 * xer_so holds the value of the XER[SO] bit.  The bit is extracted at the
 * beginning of a unit, and updated anytime XER is written, if any
 * instruction in the unit reads XER[SO] (such as an ALU instruction with
 * Rc=1).
 *
 * fr_fi_fprf holds the value of bits 13-19 of FPSCR (the FI, FR, and FPRF
 * fields of that register).  The alias is always allocated if FPSCR is
 * used by the unit, but its value is not valid unless
 * GuestPPCContext.fr_fi_fprf_loaded is true.
 */
typedef struct GuestPPCRegMap {
    uint16_t gpr[32];
    uint16_t fpr[32];
    uint16_t crb[32];
    uint16_t cr;
    uint16_t lr;
    uint16_t ctr;
    uint16_t xer;
    uint16_t xer_so;
    uint16_t fpscr;
    uint16_t fr_fi_fprf;
    uint16_t nia;
} GuestPPCRegMap;

/* Context block used to maintain translation state. */
typedef struct GuestPPCContext {
    /* Arguments passed from binrec_translate(). */
    binrec_t *handle;
    RTLUnit *unit;
    uint32_t start;

    /* True if the FORWARD_LOADS optimization is active. */
    bool forward_loads;
    /* True if the TRIM_CR_STORES optimization is active.  (Even if the
     * flag is enabled, the optimization may fail due to lack of memory,
     * or it may be suppressed by USE_SPLIT_FIELDS not being enabled.) */
    bool trim_cr_stores;
    /* True if the USE_SPLIT_FIELDS optimization is active.  (Stored here
     * just for convenience. */
    bool use_split_fields;

    /* List of basic blocks found from scanning, sorted by address. */
    GuestPPCBlockInfo *blocks;
    int num_blocks;
    int blocks_size;  // Allocated size of array.

    /* Index of current block being translated. */
    int current_block;

    /* RTL label for the unit epilogue, or 0 if none has been allocated. */
    uint16_t epilogue_label;
    /* RTL register holding the processor state block. */
    uint16_t psb_reg;
    /* RTL register holding the guest memory base address. */
    uint16_t membase_reg;

    /* Alias registers for guest CPU state. */
    GuestPPCRegMap alias;
    /* Alias registers for single-precision multiply rounding (see
     * round_for_multiply() in guest-ppc-rtl.c). */
    uint16_t alias_mulround_frA;
    uint16_t alias_mulround_frC;

    /* Set of FPR registers which need vector (paired-single) aliases. */
    uint32_t fpr_is_ps;
    /* Set of live FPR registers which need to be stored back to the state
     * block. */
    uint32_t fpr_dirty;
    /* Set of live FPR registers known to have non-SNaN values. */
    uint32_t fpr_is_safe;
    /* Same, for paired-single slot 1. */
    uint32_t ps1_is_safe;
    /* Set of CR bits which are modified by the unit.  These bits are
     * stored in the same order as the CR word, so that the MSB corresponds
     * to CR bit 0; this saves the cost of a bit-reverse operation every
     * time we need to merge bits back to the CR word.  This is always zero
     * if the USE_SPLIT_FIELDS optimization is not enabled. */
    uint32_t crb_changed_bitrev;
    /* Set of CR bits which have live registers.  Bits are in natural order
     * (the LSB corresponds to CR bit 0), as with GuestPPCBlockInfo bitmaps.
     * This is always zero if the USE_SPLIT_FIELDS optimization is not
     * enabled. */
    uint32_t crb_dirty;
    /* Flag indicating whether FPSCR is written by any instruction in the
     * unit. */
    bool fpscr_changed;
    /* Flag indicating whether the fr_fi_fprf alias contains a valid value.
     * This is always false if the USE_SPLIT_FIELDS optimization is not
     * enabled. */
    bool fr_fi_fprf_loaded;

    /* RTL registers for each CPU register live in the current block. */
    GuestPPCRegMap live;
    /* Current type of each live FPR. */
    uint8_t fpr_type[32];

    /* Most recent SET_ALIAS instruction for each register (if killable),
     * or -1 for none. */
    struct {
        int gpr[32];
        int crb[32];
        int cr;
        int lr;
        int ctr;
        int xer;
        int xer_so;
        int fpscr;
        int fr_fi_fprf;
    } last_set;

    /* RTL registers containing raw values loaded from memory for each
     * register, or 0 if none is available.  These are always zero if the
     * FORWARD_LOADS optimization is not enabled. */
    uint16_t gpr_raw[32];
    uint16_t fpr_raw[32];  // Value from lfs or lfd
    uint16_t ps_raw[32];  // Value from psq_l (if CONSTANT_GQRS)

    /* RTL register containing the compare value for a paired stwcx. in
     * big-endian byte order. */
    int paired_lwarx_data_be;

    /* True if the next instruction should be skipped.  Used when
     * optimizing a pair of instructions as a unit, such as sc followed
     * by blr (which becomes a tail call to the system call handler). */
    bool skip_next_insn;

    /* True if the warning about a floating-point instruction with Rc=1
     * with FPSCR state disabled has been logged for this unit. */
    bool warned_useless_fp_Rc;
} GuestPPCContext;

/*************************************************************************/
/******************** Internal function declarations *********************/
/*************************************************************************/

/*-------- Optimization routines (guest-ppc-optimize.c) --------*/

/**
 * guest_ppc_trim_cr_stores:  Look for stores to CR bits which are dead on
 * at least one path out of a branch instruction, and kill them or move
 * them onto the appropriate code path.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     BO: BO field of the branch instruction (0x14 for an unconditional
 *         branch).
 *     BI: BI field of the branch instruction (ignored for unconditional
 *         branches).
 *     crb_store_branch_ret: Pointer to variable to receive the bitmask of
 *         CR bits (LSB = CR bit 0) to be stored on the branch-taken code
 *         path only.  May be NULL for unconditional branches.
 *     crb_store_next_ret: Pointer to variable to receive the bitmask of
 *         CR bits (LSB = CR bit 0) to be stored on the branch-not-taken
 *         code path only.  May be NULL for unconditional branches.
 *     crb_reg_ret: Pointer to 32-element array to receive the RTL register
 *         containing the value for each set bit in crb_store_*_ret.  May
 *         be NULL for unconditional branches.
 *     crb_insn_ret: Pointer to 32-element array to receive the RTL
 *         instruction which sets the value for each set bit in
 *         crb_store_*_ret.  An opcode value of zero indicates that no
 *         setting instruction should be added on the relevant code path.
 *         May be NULL for unconditional branches.
 */
#define guest_ppc_trim_cr_stores INTERNAL(guest_ppc_trim_cr_stores)
extern void guest_ppc_trim_cr_stores(
    GuestPPCContext *ctx, int BO, int BI, uint32_t *crb_store_branch_ret,
    uint32_t *crb_store_next_ret, uint16_t *crb_reg_ret,
    struct RTLInsn *crb_insn_ret);

/*-------- PowerPC to RTL translation (guest-ppc-rtl.c) --------*/

/**
 * guest_ppc_translate_block:  Generate RTL for the selected basic block.
 *
 * [Parameters]
 *     context: Translation context.
 *     index: Block index (into context->blocks[]).
 * [Return value]
 *     True on success, false on error.
 */
#define guest_ppc_translate_block INTERNAL(guest_ppc_translate_block)
extern bool guest_ppc_translate_block(GuestPPCContext *ctx, int index);

/**
 * guest_ppc_flush_cr:  Flush all CR bit aliases to the full CR register.
 *
 * This function does nothing if the BINREC_OPT_G_PPC_USE_SPLIT_FIELDS
 * optimization is not enabled.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     make_live: True to leave the flushed CR value live in its alias,
 *         false to leave the alias liveness state unchanged.
 */
#define guest_ppc_flush_cr INTERNAL(guest_ppc_flush_cr)
extern void guest_ppc_flush_cr(GuestPPCContext *ctx, bool make_live);

/**
 * guest_ppc_flush_fpscr:  Flush the FR/FI/FPRF alias to the full FPSCR
 * register.  FPSCR is not made live if it was not live already.
 *
 * This function does nothing if the BINREC_OPT_G_PPC_USE_SPLIT_FIELDS
 * optimization is not enabled.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
#define guest_ppc_flush_fpscr INTERNAL(guest_ppc_flush_fpscr)
extern void guest_ppc_flush_fpscr(GuestPPCContext *ctx);

/*-------- Input code scanning (guest-ppc-scan.c) --------*/

/**
 * guest_ppc_scan:  Scan guest memory to find the range of addresses to
 * translate.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     limit: Upper address bound for scanning.
 * [Return value]
 *     True on success, false on error.
 */
#define guest_ppc_scan INTERNAL(guest_ppc_scan)
extern bool guest_ppc_scan(GuestPPCContext *ctx, uint32_t limit);

/*-------- Miscellaneous utility routines (guest-ppc-translate.c) --------*/

/**
 * guest_ppc_get_epilogue_label:  Return the RTL label for the epilogue,
 * allocating it if necessary.
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     Epilogue label.
 */
#define guest_ppc_get_epilogue_label INTERNAL(guest_ppc_get_epilogue_label)
extern int guest_ppc_get_epilogue_label(GuestPPCContext *ctx);

/**
 * guest_ppc_get_insn_at:  Return the instruction word at the given address,
 * or zero if the given address is outside the given block.  The address is
 * assumed to be properly aligned.
 */
static inline PURE_FUNCTION uint32_t guest_ppc_get_insn_at(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address)
{
    ASSERT((address & 3) == 0);
    if (address - block->start < block->len) {
        const uint32_t *memory_base =
            (const uint32_t *)ctx->handle->setup.guest_memory_base;
        return bswap_be32(memory_base[address/4]);
    } else {
        return 0;
    }
}

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_GUEST_PPC_INTERNAL_H
