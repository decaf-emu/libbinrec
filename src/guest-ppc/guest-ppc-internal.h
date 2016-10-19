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
#include "src/rtl.h"

/*************************************************************************/
/*********************** Architectural constants *************************/
/*************************************************************************/

/* XER register bits. */
#define XER_SO_SHIFT  31
#define XER_OV_SHIFT  30
#define XER_CA_SHIFT  29
#define XER_SO  (1u << XER_SO_SHIFT)
#define XER_OV  (1u << XER_OV_SHIFT)
#define XER_CA  (1u << XER_CA_SHIFT)

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
#define FPSCR_FX      (1u << FPSCR_FX_SHIFT)
#define FPSCR_FEX     (1u << FPSCR_FEX_SHIFT)
#define FPSCR_VX      (1u << FPSCR_VX_SHIFT)
#define FPSCR_OX      (1u << FPSCR_OX_SHIFT)
#define FPSCR_UX      (1u << FPSCR_UX_SHIFT)
#define FPSCR_ZX      (1u << FPSCR_ZX_SHIFT)
#define FPSCR_XX      (1u << FPSCR_XX_SHIFT)
#define FPSCR_VXSNAN  (1u << FPSCR_VXSNAN_SHIFT)
#define FPSCR_VXISI   (1u << FPSCR_VXISI_SHIFT)
#define FPSCR_VXIDI   (1u << FPSCR_VXIDI_SHIFT)
#define FPSCR_VXZDZ   (1u << FPSCR_VXZDZ_SHIFT)
#define FPSCR_VXIMZ   (1u << FPSCR_VXIMZ_SHIFT)
#define FPSCR_VXVC    (1u << FPSCR_VXVC_SHIFT)
#define FPSCR_FR      (1u << FPSCR_FR_SHIFT)
#define FPSCR_FI      (1u << FPSCR_FI_SHIFT)
#define FPSCR_FPRF    (31u << FPSCR_FPRF_SHIFT)
#define FPSCR_VXSOFT  (1u << FPSCR_VXSOFT_SHIFT)
#define FPSCR_VXSQRT  (1u << FPSCR_VXSQRT_SHIFT)
#define FPSCR_VXCVI   (1u << FPSCR_VXCVI_SHIFT)
#define FPSCR_VE      (1u << FPSCR_VE_SHIFT)
#define FPSCR_OE      (1u << FPSCR_OE_SHIFT)
#define FPSCR_UE      (1u << FPSCR_UE_SHIFT)
#define FPSCR_ZE      (1u << FPSCR_ZE_SHIFT)
#define FPSCR_XE      (1u << FPSCR_XE_SHIFT)
#define FPSCR_NI      (1u << FPSCR_NI_SHIFT)
#define FPSCR_RN      (3u << FPSCR_RN_SHIFT)

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

/* Scan data for a single basic block. */
typedef struct GuestPPCBlockInfo {
    /* Start address and length (in bytes) of the block. */
    uint32_t start;
    uint32_t len;

    /* Bitmasks of registers which are used (i.e., their values on block
     * entry are read) and changed by the block. */
    uint32_t gpr_used;
    uint32_t gpr_changed;
    uint32_t fpr_used;
    uint32_t fpr_changed;
    uint32_t crb_used;
    uint32_t crb_changed;
    uint8_t
        cr_used : 1,  // For insns which access CR as a whole (mfcr/mtcr).
        lr_used : 1,
        ctr_used : 1,
        xer_used : 1,
        fpscr_used : 1;
    uint8_t
        cr_changed : 1,
        lr_changed : 1,
        ctr_changed : 1,
        xer_changed : 1,
        fpscr_changed : 1;

    /* Is this block a branch target?  (Labels are only allocated for
     * branch targets.) */
    bool is_branch_target;

    /* RTL label for this block, or 0 if none has been allocated. */
    int label;
} GuestPPCBlockInfo;

/* RTL register set corresponding to guest CPU state.  CR is recorded as
 * both 32 1-bit aliases (crb[]) and one 32-bit alias (cr); the semantics
 * of their interaction are that (1) the base value of CR is the value of
 * the RTL alias GuestPPCContext.alias.cr, and (2) for each bit b in CR, if
 * the corresponding bit is set in GuestPPCContext.crb_loaded, the value of
 * the bit is the value of the RTL alias GuestPPCContext.crb[b], else the
 * value of the bit is the value of the corresponding bit in the base value
 * of CR as determined above. */
typedef struct GuestPPCRegSet {
    uint16_t gpr[32];
    uint16_t fpr[32];
    uint16_t crb[32];
    uint16_t cr;
    uint16_t lr;
    uint16_t ctr;
    uint16_t xer;
    uint16_t fpscr;
    uint16_t nia;
} GuestPPCRegSet;

/* Context block used to maintain translation state. */
typedef struct GuestPPCContext {
    /* Arguments passed from binrec_translate(). */
    binrec_t *handle;
    RTLUnit *unit;
    uint32_t start;

    /* List of basic blocks found from scanning, sorted by address. */
    GuestPPCBlockInfo *blocks;
    int num_blocks;
    int blocks_size;  // Allocated size of array.

    /* RTL label for the unit epilogue, or 0 if none has been allocated. */
    uint16_t epilogue_label;
    /* RTL register holding the processor state block. */
    uint16_t psb_reg;
    /* RTL register holding the guest memory base address. */
    uint16_t membase_reg;

    /* Alias registers for guest CPU state. */
    GuestPPCRegSet alias;

    /* Set of CR bits which are modified by the unit.  These bits are
     * stored in the same order as the CR word, so that the MSB corresponds
     * to CR bit 0. */
    uint32_t crb_changed;
    /* Set of CR bits which have been loaded into bit aliases, in the same
     * bit order as crb_loaded. */
    uint32_t crb_loaded;

    /* RTL registers for each CPU register live in the current block. */
    GuestPPCRegSet live;

    /* Dirty state of live registers. */
    uint32_t gpr_dirty;
    uint32_t fpr_dirty;
    uint32_t crb_dirty;
    uint8_t
        cr_dirty : 1,
        lr_dirty : 1,
        ctr_dirty : 1,
        xer_dirty : 1,
        fpscr_dirty : 1;

    /* True if the next instruction should be skipped.  Used when
     * optimizing a pair of instructions as a unit, such as sc followed
     * by blr (which becomes a tail call to the system call handler). */
    bool skip_next_insn;
} GuestPPCContext;

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

/**
 * guest_ppc_flush_cr:  Flush all CR bit aliases to the full CR register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     make_live: True to leave the flushed CR value live in its alias,
 *         false to leave the alias liveness state unchanged.
 */
#define guest_ppc_flush_cr INTERNAL(guest_ppc_flush_cr)
extern void guest_ppc_flush_cr(GuestPPCContext *ctx, bool make_live);

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

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_GUEST_PPC_INTERNAL_H
