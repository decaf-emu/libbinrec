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
/************************ Register bit constants *************************/
/*************************************************************************/

#define XER_SO_SHIFT  31
#define XER_OV_SHIFT  30
#define XER_CA_SHIFT  29
#define XER_SO  (1u << XER_SO_SHIFT)
#define XER_OV  (1u << XER_OV_SHIFT)
#define XER_CA  (1u << XER_CA_SHIFT)

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

#define FPSCR_RN_N    0  // Round to nearest.
#define FPSCR_RN_Z    1  // Round toward zero.
#define FPSCR_RN_P    2  // Round toward plus infinity.
#define FPSCR_RN_M    3  // Round toward minus infinity.

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
    uint8_t cr_used;
    uint8_t cr_changed;
    uint8_t
        lr_used : 1,
        ctr_used : 1,
        xer_used : 1,
        fpscr_used : 1,
        reserve_used : 1;
    uint8_t
        lr_changed : 1,
        ctr_changed : 1,
        xer_changed : 1,
        fpscr_changed : 1,
        reserve_changed : 1;

    /* RTL label for this block, or 0 if none has been allocated yet. */
    int label;
} GuestPPCBlockInfo;

/* RTL register set corresponding to guest CPU state. */
typedef struct GuestPPCRegSet {
    uint16_t gpr[32];
    uint16_t fpr[32];
    uint16_t cr[8];
    uint16_t lr;
    uint16_t ctr;
    uint16_t xer;
    uint16_t fpscr;
    uint16_t reserve_flag;
    uint16_t reserve_address;
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

    /* RTL label for the unit epilogue. */
    uint16_t epilogue_label;

    /* RTL register holding the processor state block. */
    uint16_t psb_reg;

    /* Alias registers for guest CPU state. */
    GuestPPCRegSet alias;

    /* RTL registers for each CPU register live in the current block. */
    GuestPPCRegSet live;

    /* Merged register change state from all basic blocks. */
    uint32_t gpr_changed;
    uint32_t fpr_changed;
    uint8_t cr_changed;
    uint8_t
        lr_changed : 1,
        ctr_changed : 1,
        xer_changed : 1,
        fpscr_changed : 1,
        reserve_changed : 1;
} GuestPPCContext;

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

/**
 * guest_ppc_flush_state:  Flush all cached state (in RTL aliases) to the
 * processor state block.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
#define guest_ppc_flush_state INTERNAL(guest_ppc_flush_state)
extern void guest_ppc_flush_state(GuestPPCContext *ctx);

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
