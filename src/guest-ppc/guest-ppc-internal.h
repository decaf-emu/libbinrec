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
 * guest_ppc_translate_block: Generate RTL for the selected basic block.
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
 * guest_ppc_scan: Scan guest memory to find the range of addresses to
 * translate.
 *
 * [Parameters]
 *     context: Translation context.
 *     limit: Upper address bound for scanning.
 * [Return value]
 *     True on success, false on error.
 */
#define guest_ppc_scan INTERNAL(guest_ppc_scan)
extern bool guest_ppc_scan(GuestPPCContext *ctx, uint32_t limit);

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_GUEST_PPC_INTERNAL_H
