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

    /* Allocate a label for each basic block that is a branch target.
     * We don't allocate a label for the epilogue unless and until it
     * ends up being needed, so as not to unnecessarily break the code
     * into separate RTL blocks. */
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

    /* Allocate and initialize a register for the host memory base. */
    ctx->membase_reg = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD_ARG, ctx->membase_reg, 0, 0, 1);
    rtl_make_unique_pointer(unit, ctx->membase_reg);
    rtl_make_unspillable(unit, ctx->membase_reg);
    rtl_set_membase_pointer(unit, ctx->membase_reg);

    /* Allocate an alias register for the output NIA. */
    ctx->alias.nia = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
    rtl_set_alias_storage(unit, ctx->alias.nia, ctx->psb_reg,
                          ctx->handle->setup.state_offsets_ppc.nia);

    /* Check which guest CPU registers are referenced in the unit, to avoid
     * creating unnecessary RTL alias registers below. */
    GuestPPCRegSet touched;
    memset(&touched, 0, sizeof(touched));
    ctx->fpr_is_ps = 0;
    uint32_t crb_changed = 0;
    ctx->fpscr_changed = false;
    for (int i = 0; i < ctx->num_blocks; i++) {
        for (int j = 0; j < (int)sizeof(touched) / 4; j++) {
            (&touched.gpr)[j] |= (&ctx->blocks[i].touched.gpr)[j];
        }
        ctx->fpr_is_ps |= ctx->blocks[i].fpr_is_ps;
        crb_changed |= ctx->blocks[i].crb_changed;
        ctx->fpscr_changed |= ctx->blocks[i].fpscr_changed;
    }
    if (ctx->use_split_fields) {
        ctx->crb_changed_bitrev = bitrev32(crb_changed);
    }

    /* Allocate alias registers for all required guest registers. */

    for (int i = 0; i < 32; i++) {
        if (touched.gpr & (1 << i)) {
            ctx->alias.gpr[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
            rtl_set_alias_storage(unit, ctx->alias.gpr[i], ctx->psb_reg,
                                  ctx->handle->setup.state_offsets_ppc.gpr + i*4);
        }
    }

    for (int i = 0; i < 32; i++) {
        if (touched.fpr & (1 << i)) {
            if (ctx->fpr_is_ps & (1 << i)) {
                ctx->alias.fpr[i] =
                    rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT64);
            } else {
                ctx->alias.fpr[i] =
                    rtl_alloc_alias_register(unit, RTLTYPE_FLOAT64);
            }
            rtl_set_alias_storage(unit, ctx->alias.fpr[i], ctx->psb_reg,
                                  ctx->handle->setup.state_offsets_ppc.fpr + i*16);
        }
    }

    if (touched.crb || touched.cr) {
        ctx->alias.cr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.cr, ctx->psb_reg,
                              ctx->handle->setup.state_offsets_ppc.cr);
    }

    if (ctx->use_split_fields) {
        int cr = 0;
        for (int i = 0; i < 32; i++) {
            /* Bits which aren't changed are re-extracted from CR as needed. */
            if (crb_changed & (1 << i)) {
                ctx->alias.crb[i] =
                    rtl_alloc_alias_register(unit, RTLTYPE_INT32);
                /* Load the initial value of the bit, unless we know from
                 * TRIM_CR_STORES data flow analysis that its value is not
                 * needed.  We have to do this here in case the bit is
                 * first loaded in a block which is only conditionally
                 * executed; in that case, if the block was skipped, the
                 * alias would remain uninitialized.  If the value is not
                 * in fact needed, data flow analysis at the RTL level
                 * should be able to eliminate this initialization. */
                bool need_load;
                if (ctx->trim_cr_stores) {
                    need_load =
                        !(ctx->blocks[0].crb_changed_recursive & (1 << i));
                } else {
                    need_load = true;
                }
                if (need_load) {
                    if (!cr) {
                        cr = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                                     cr, 0, 0, ctx->alias.cr);
                    }
                    const int crb = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb, cr, 0, (31-i) | (1<<8));
                    rtl_add_insn(unit, RTLOP_SET_ALIAS,
                                 0, crb, 0, ctx->alias.crb[i]);
                }
            }
        }
    }

    if (touched.lr) {
        ctx->alias.lr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.lr, ctx->psb_reg,
                              ctx->handle->setup.state_offsets_ppc.lr);
    }

    if (touched.ctr) {
        ctx->alias.ctr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.ctr, ctx->psb_reg,
                              ctx->handle->setup.state_offsets_ppc.ctr);
    }

    if (touched.xer) {
        ctx->alias.xer = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.xer, ctx->psb_reg,
                              ctx->handle->setup.state_offsets_ppc.xer);
        if (ctx->use_split_fields && touched.xer_so) {
            ctx->alias.xer_so = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
            /* As with CR bit aliases, this initialization may not be
             * necessary, but RTL data flow analysis should be able to
             * eliminate it in such a case. */
            const int xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
            const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT, so, xer, 0, XER_SO_SHIFT | 1<<8);
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, so, 0, ctx->alias.xer_so);
        }
    }

    if (touched.fpscr) {
        ctx->alias.fpscr = rtl_alloc_alias_register(unit, RTLTYPE_INT32);
        rtl_set_alias_storage(unit, ctx->alias.fpscr, ctx->psb_reg,
                              ctx->handle->setup.state_offsets_ppc.fpscr);
        if (ctx->use_split_fields && touched.fr_fi_fprf) {
            ctx->alias.fr_fi_fprf =
                rtl_alloc_alias_register(unit, RTLTYPE_INT32);
            const int fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
            const int fff = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT,
                         fff, fpscr, 0, FPSCR_FPRF_SHIFT | 7<<8);
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, fff, 0, ctx->alias.fr_fi_fprf);
        }
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
    guest_ppc_flush_fpscr(ctx);
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
    ctx.forward_loads =
        (handle->guest_opt & BINREC_OPT_G_PPC_FORWARD_LOADS) != 0;
    ctx.use_split_fields =
        (handle->guest_opt & BINREC_OPT_G_PPC_USE_SPLIT_FIELDS) != 0;

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
