/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/bitutils.h"
#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl.h"

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * init_unit:  Set up the context's RTLUnit for translation.  This creates
 * a label for each basic block identified during scanning as well as for
 * the unit epilogue, allocates alias registers for each guest CPU register
 * used by the unit, and loads initial register values needed by the unit.
 *
 * Returns true on success, false on error.
 */
static bool init_unit(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    /* Allocate a label for each basic block that is a branch target
     * We don't allocate a label for the epilogue unless and until it
     * ends up being needed. */
    for (int i = 0; i < ctx->num_blocks; i++) {
        if (ctx->blocks[i].is_branch_target) {
            ctx->blocks[i].label = rtl_alloc_label(unit);
        }
    }

    /* Allocate a register for the guest processor state block, and fetch
     * the state block pointer from the function parameter. */
    ctx->psb_reg = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD_ARG, ctx->psb_reg, 0, 0, 0);
    rtl_make_unique_pointer(unit, ctx->psb_reg);

    /* Allocate and initialize a register for the host memory base.  Make
     * it unfoldable since that will generally result in better host code
     * (base with 32-bit displacement instead of absolute 64-bit address). */
    ctx->membase_reg = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, ctx->membase_reg, 0, 0,
                 ctx->handle->setup.host_memory_base);
    rtl_make_unique_pointer(unit, ctx->membase_reg);
    rtl_make_unfoldable(unit, ctx->membase_reg);

    /* Allocate an alias register for the output NIA. */
    ctx->alias.nia = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
    rtl_set_alias_storage(unit, ctx->alias.nia, ctx->psb_reg,
                          ctx->handle->setup.state_offset_nia);

    /* Check which guest CPU registers are referenced in the unit, to avoid
     * creating unnecessary RTL alias registers below. */
    uint32_t gpr_used = 0;
    uint32_t gpr_changed = 0;
    uint32_t fpr_used = 0;
    uint32_t fpr_changed = 0;
    uint32_t crb_used = 0;
    uint32_t crb_changed = 0;
    bool cr_used = false;
    bool lr_used = false;
    bool ctr_used = false;
    bool xer_used = false;
    bool fpscr_used = false;
    bool cr_changed = false;
    bool lr_changed = false;
    bool ctr_changed = false;
    bool xer_changed = false;
    bool fpscr_changed = false;
    for (int i = 0; i < ctx->num_blocks; i++) {
        gpr_used |= ctx->blocks[i].gpr_used;
        gpr_changed |= ctx->blocks[i].gpr_changed;
        fpr_used |= ctx->blocks[i].fpr_used;
        fpr_changed |= ctx->blocks[i].fpr_changed;
        crb_used |= ctx->blocks[i].crb_used;
        crb_changed |= ctx->blocks[i].crb_changed;
        cr_used |= ctx->blocks[i].cr_used;
        cr_changed |= ctx->blocks[i].cr_changed;
        lr_used |= ctx->blocks[i].lr_used;
        lr_changed |= ctx->blocks[i].lr_changed;
        ctr_used |= ctx->blocks[i].ctr_used;
        ctr_changed |= ctx->blocks[i].ctr_changed;
        xer_used |= ctx->blocks[i].xer_used;
        xer_changed |= ctx->blocks[i].xer_changed;
        fpscr_used |= ctx->blocks[i].fpscr_used;
        fpscr_changed |= ctx->blocks[i].fpscr_changed;
    }
    ctx->crb_changed = bitrev32(crb_changed);

    /* Allocate alias registers for all required guest registers. */

    for (int i = 0; i < 32; i++) {
        if ((gpr_used | gpr_changed) & (1 << i)) {
            ctx->alias.gpr[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
            rtl_set_alias_storage(unit, ctx->alias.gpr[i], ctx->psb_reg,
                                  ctx->handle->setup.state_offset_gpr + i*4);
        }
    }

    for (int i = 0; i < 32; i++) {
        if ((fpr_used | fpr_changed) & (1 << i)) {
            ctx->alias.fpr[i] =
                rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_set_alias_storage(unit, ctx->alias.fpr[i], ctx->psb_reg,
                                  ctx->handle->setup.state_offset_fpr + i*16);
        }
    }

    for (int i = 0; i < 32; i++) {
        /* Bits which aren't changed are re-extracted from CR as needed. */
        if (crb_changed & (1 << i)) {
            ctx->alias.crb[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        }
    }

    if (crb_used || crb_changed || cr_used || cr_changed) {
        ctx->alias.cr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.cr, ctx->psb_reg,
                              ctx->handle->setup.state_offset_cr);
    }

    if (lr_used || lr_changed) {
        ctx->alias.lr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.lr, ctx->psb_reg,
                              ctx->handle->setup.state_offset_lr);
    }

    if (ctr_used || ctr_changed) {
        ctx->alias.ctr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.ctr, ctx->psb_reg,
                              ctx->handle->setup.state_offset_ctr);
    }

    if (xer_used || xer_changed) {
        ctx->alias.xer = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.xer, ctx->psb_reg,
                              ctx->handle->setup.state_offset_xer);
    }

    if (fpscr_used || fpscr_changed) {
        ctx->alias.fpscr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.fpscr, ctx->psb_reg,
                              ctx->handle->setup.state_offset_fpscr);
    }

    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to generate prologue");
        return false;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * add_epilogue:  Adds the unit epilogue to the context's RTLUnit,
 * consisting of stores to write modified guest registers back to the
 * processor state block (for cases not automatically handled by RTL alias
 * writeback) and a final return instruction to return to the unit's caller.
 *
 * Returns true on success, false on error.
 */
static bool add_epilogue(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    if (ctx->epilogue_label) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, ctx->epilogue_label);
    }
    guest_ppc_flush_cr(ctx, false);
    rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);

    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to generate epilogue");
        return false;
    }

    return true;
}

/*************************************************************************/
/************************ Translation entry point ************************/
/*************************************************************************/

bool guest_ppc_translate(binrec_t *handle, uint32_t address,
                         uint32_t limit, RTLUnit *unit)
{
    ASSERT(handle);
    ASSERT(unit);

    /* PowerPC code must be word-aligned. */
    if (UNLIKELY(address & 3)) {
        log_error(handle, "Invalid address 0x%X (not word-aligned)", address);
        return 0;
    }

    if (UNLIKELY(address + 3 > handle->code_range_end)) {
        log_error(handle, "First instruction at 0x%X falls outside code range",
                  address);
        return 0;
    }
    if (UNLIKELY(address + 3 > limit)) {
        log_error(handle, "First instruction at 0x%X falls outside translation"
                  " range", address);
        return 0;
    }

    GuestPPCContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.handle = handle;
    ctx.unit = unit;
    ctx.start = address;
    ctx.blocks = NULL;

    /* Scan guest memory to determine the range of code to translate and
     * record relevant properties about the code. */
    if (!guest_ppc_scan(&ctx, limit)) {
        goto error;
    }
    ASSERT(ctx.num_blocks > 0);

    /* Initialize the RTL unit for translation. */
    if (!init_unit(&ctx)) {
        goto error;
    }

    /* Translate the code into RTL.  Note that the block list may be
     * expanded during translation if a jump table is detected, but
     * blocks will never be inserted before the current block. */
    for (int i = 0; i < ctx.num_blocks; i++) {
        if (!guest_ppc_translate_block(&ctx, i)) {
            goto error;
        }
    }

    /* Append the epilogue, storing all modified registers back to the
     * processor state block and returning to the caller. */
    if (!add_epilogue(&ctx)) {
        goto error;
    }

    binrec_free(ctx.handle, ctx.blocks);
    return true;

  error:
    binrec_free(ctx.handle, ctx.blocks);
    return false;
}

/*************************************************************************/
/*************************************************************************/
