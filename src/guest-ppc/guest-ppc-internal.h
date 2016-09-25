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
/************************** Convenience macros ***************************/
/*************************************************************************/

/*
 * These macros all assume the variable "ctx" points to the current
 * translation context and "unit" points to the corresponding RTLUnit.
 */

/**
 * ICE_FORMAT:  Format string and arguments for reporting an error from
 * one of the following macros.  "text" must be a literal string.  To
 * simply append text (and optionally format arguments) to the message,
 * leave this definition unchanged and change the definition of ICE_SUFFIX
 * and ICE_SUFFIX_ARGS instead.
 */
#define ICE_FORMAT(text) \
    "Internal compiler error: %s:%d: " text ICE_SUFFIX, \
    strrchr(__FILE__,'/') ? strrchr(__FILE__,'/') + 1 : __FILE__, __LINE__ \
    ICE_SUFFIX_ARGS

/**
 * ICE_SUFFIX:  Format string suffix (and any arguments required) for error
 * messages from one of the following macros.
 */
#define ICE_SUFFIX  /* nothing */

/**
 * ADD_INSN:  Call rtl_add_insn(unit, ...) and return false from the current
 * function on failure.
 */
#define ADD_INSN(opcode, dest, src1, src2, other)  do {         \
    if (UNLIKELY(!rtl_add_insn(unit, (opcode), (dest), (src1),  \
                               (src2), (other)))) {             \
        log_ice(ctx->handle, "Failed to add instruction" ICE_SUFFIX); \
        return false;                                           \
    }                                                           \
} while (0)

/**
 * ALLOC_LABEL:  Call rtl_alloc_label(unit) and assign the result to the
 * given variable, returning false from the current function on failure.
 */
#define ALLOC_LABEL(var)  do {                                  \
    if (UNLIKELY(!(var = rtl_alloc_label(unit)))) {             \
        log_ice(ctx->handle, "Failed to allocate label" ICE_SUFFIX); \
        return false;                                           \
    }                                                           \
} while (0)

/**
 * ALLOC_ALIAS:  Call rtl_alloc_alias_register(unit, ...) and assign the
 * result to the given variable, returning false from the current function
 * on failure.
 */
#define ALLOC_ALIAS(var, type)  do {                            \
    if (UNLIKELY(!(var = rtl_alloc_alias_register(unit, (type))))) {  \
        log_ice(ctx->handle, "Failed to allocate register" ICE_SUFFIX); \
        return false;                                           \
    }                                                           \
} while (0)

/**
 * ALLOC_REGISTER:  Call rtl_alloc_register(unit, ...) and assign the
 * result to the given variable, returning false from the current function
 * on failure.
 */
#define ALLOC_REGISTER(var, type)  do {                         \
    if (UNLIKELY(!(var = rtl_alloc_register(unit, (type))))) {  \
        log_ice(ctx->handle, "Failed to allocate register" ICE_SUFFIX); \
        return false;                                           \
    }                                                           \
} while (0)

/**
 * DECLARE_NEW_REGISTER:  Declare the given name as a variable and assign
 * it a newly-allocated RTL register of the given type, returning false
 * from the current function on failure.
 */
#define DECLARE_NEW_REGISTER(name, type)                        \
    const int name = rtl_alloc_register(unit, (type));          \
    if (UNLIKELY(!name)) {                                      \
        log_ice(ctx->handle, "Failed to allocate register" ICE_SUFFIX); \
        return false;                                           \
    }

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

/**
 * guest_ppc_gen_rtl: Generate RTL for the selected basic block.
 *
 * [Parameters]
 *     context: Translation context.
 *     index: Block index (into context->blocks[]).
 * [Return value]
 *     True on success, false on error.
 */
#define guest_ppc_gen_rtl INTERNAL(guest_ppc_gen_rtl)
extern bool guest_ppc_gen_rtl(GuestPPCContext *ctx, int index);

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
