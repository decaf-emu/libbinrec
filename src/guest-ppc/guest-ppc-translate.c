/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl.h"

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

#undef ICE_SUFFIX
#define ICE_SUFFIX  " in prologue"

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

    /* Allocate a label for each basic block and for the epilogue. */
    for (int i = 0; i < ctx->num_blocks; i++) {
        ALLOC_LABEL(ctx->blocks[i].label);
    }
    ALLOC_LABEL(ctx->epilogue_label);

    /* Allocate a register for the guest processor state block, and fetch
     * the state block pointer from the function parameter. */
    ALLOC_REGISTER(ctx->psb_reg, RTLTYPE_ADDRESS);
    rtl_make_unique_pointer(unit, ctx->psb_reg);
    ADD_INSN(RTLOP_LOAD_ARG, ctx->psb_reg, 0, 0, 0);

    /* Allocate an alias register for the output NIA. */
    ALLOC_ALIAS(ctx->alias.nia, RTLTYPE_INT32);

    /* Check which guest CPU registers are referenced in the unit, to avoid
     * creating unnecessary RTL alias registers below.  We accumulate the
     * bitmasks in local variables to avoid writes through ctx->blocks[]
     * causing optimization barriers, and also since we don't need the used
     * bitmasks past the end of this function. */
    uint32_t gpr_used = 0;
    uint32_t gpr_changed = 0;
    uint32_t fpr_used = 0;
    uint32_t fpr_changed = 0;
    uint8_t cr_used = 0;
    uint8_t cr_changed = 0;
    bool lr_used = false;
    bool ctr_used = false;
    bool xer_used = false;
    bool fpscr_used = false;
    bool reserve_used = false;
    bool lr_changed = false;
    bool ctr_changed = false;
    bool xer_changed = false;
    bool fpscr_changed = false;
    bool reserve_changed = false;
    for (int i = 0; i < ctx->num_blocks; i++) {
        gpr_used |= ctx->blocks[i].gpr_used;
        gpr_changed |= ctx->blocks[i].gpr_changed;
        fpr_used |= ctx->blocks[i].fpr_used;
        fpr_changed |= ctx->blocks[i].fpr_changed;
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
        reserve_used |= ctx->blocks[i].reserve_used;
        reserve_changed |= ctx->blocks[i].reserve_changed;
    }
    ctx->gpr_changed = gpr_changed;
    ctx->fpr_changed = fpr_changed;
    ctx->cr_changed = cr_changed;
    ctx->lr_changed = lr_changed;
    ctx->ctr_changed = ctr_changed;
    ctx->xer_changed = xer_changed;
    ctx->fpscr_changed = fpscr_changed;

    /* Allocate alias registers for all required guest registers, and load
     * registers whose initial values are needed.  We don't perform data
     * flow analysis here to match changed registers in one block with used
     * registers in a later block, so we may load more registers than
     * necessary, but RTL optimization can get rid of what's not needed. */
    for (int i = 0; i < 32; i++) {
        if ((gpr_used | gpr_changed) & (1 << i)) {
            ALLOC_ALIAS(ctx->alias.gpr[i], RTLTYPE_INT32);
            if (gpr_used & (1 << i)) {
                DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
                ADD_INSN(RTLOP_LOAD_I32, reg, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_gpr + i*4);
                ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.gpr[i], reg, 0, 0);
            }
        }

        if ((fpr_used | fpr_changed) & (1 << i)) {
            ALLOC_ALIAS(ctx->alias.fpr[i], RTLTYPE_V2_DOUBLE);
            if (fpr_used & (1 << i)) {
                // FIXME: load value
            }
        }
    }

    uint32_t cr = 0;
    for (int i = 0; i < 8; i++) {
        if ((cr_used | cr_changed) & (1 << i)) {
            ALLOC_ALIAS(ctx->alias.cr[i], RTLTYPE_INT32);
            if (cr_used & (1 << i)) {
                if (!cr) {
                    ALLOC_REGISTER(cr, RTLTYPE_INT32);
                    ADD_INSN(RTLOP_LOAD_I32, cr, ctx->psb_reg, 0,
                             ctx->handle->setup.state_offset_cr);
                }
                DECLARE_NEW_REGISTER(crN, RTLTYPE_INT32);
                ADD_INSN(RTLOP_BFEXT, crN, cr, 0, ((7 - i) * 4) | 4<<8);
            }
        }
    }

    if (lr_used || lr_changed) {
        ALLOC_ALIAS(ctx->alias.lr, RTLTYPE_INT32);
        if (lr_used) {
            DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_I32, reg, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_lr);
            ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.lr, reg, 0, 0);
        }
    }

    if (ctr_used || ctr_changed) {
        ALLOC_ALIAS(ctx->alias.ctr, RTLTYPE_INT32);
        if (ctr_used) {
            DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_I32, reg, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_ctr);
            ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.ctr, reg, 0, 0);
        }
    }

    if (xer_used || xer_changed) {
        ALLOC_ALIAS(ctx->alias.xer, RTLTYPE_INT32);
        if (xer_used) {
            DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_I32, reg, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_xer);
            ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.xer, reg, 0, 0);
        }
    }

    if (fpscr_used || fpscr_changed) {
        ALLOC_ALIAS(ctx->alias.fpscr, RTLTYPE_INT32);
        if (fpscr_used) {
            DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_I32, reg, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_fpscr);
            ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.fpscr, reg, 0, 0);
        }
    }

    if (reserve_used || reserve_changed) {
        ALLOC_ALIAS(ctx->alias.reserve_flag, RTLTYPE_INT32);
        ALLOC_ALIAS(ctx->alias.reserve_address, RTLTYPE_INT32);
        if (reserve_used) {
            DECLARE_NEW_REGISTER(flag, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_U8, flag, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_reserve_flag);
            ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.reserve_flag, flag, 0, 0);
            DECLARE_NEW_REGISTER(address, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_I32, address, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_reserve_address);
            ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.reserve_address, address,
                     0, 0);
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

#undef ICE_SUFFIX
#define ICE_SUFFIX  " in epilogue"

/**
 * add_epilogue:  Adds the unit epilogue to the context's RTLUnit,
 * consisting of stores to write modified guest registers back to the
 * processor state block and a final return instruction to return to the
 * unit's caller.
 *
 * Returns true on success, false on error.
 */
static bool add_epilogue(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    ADD_INSN(RTLOP_LABEL, 0, 0, 0, ctx->epilogue_label);

    for (int i = 0; i < 32; i++) {
        if (ctx->gpr_changed & (1 << i)) {
            DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
            ADD_INSN(RTLOP_GET_ALIAS, reg, ctx->alias.gpr[i], 0, 0);
            ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, reg, 0,
                     ctx->handle->setup.state_offset_gpr + i*4);
        }

        if (ctx->fpr_changed & (1 << i)) {
            // FIXME: store value
        }
    }

    if (ctx->cr_changed) {
        uint32_t cr;
        ALLOC_REGISTER(cr, RTLTYPE_INT32);
        if (ctx->cr_changed != 0xFF) {
            ADD_INSN(RTLOP_LOAD_I32, cr, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_cr);
            uint32_t mask_val = 0;
            for (int i = 0; i < 8; i++) {
                if (!(ctx->cr_changed & (1 << i))) {
                    mask_val |= 0xF << ((7 - i) * 4);
                }
            }
            DECLARE_NEW_REGISTER(mask, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_IMM, mask, 0, 0, mask_val);
            DECLARE_NEW_REGISTER(new_cr, RTLTYPE_INT32);
            ADD_INSN(RTLOP_AND, new_cr, cr, mask, 0);
            cr = new_cr;
        }
        for (int i = 0; i < 8; i++) {
            if (ctx->cr_changed & (1 << i)) {
                DECLARE_NEW_REGISTER(crN, RTLTYPE_INT32);
                ADD_INSN(RTLOP_GET_ALIAS, crN, ctx->alias.cr[i], 0, 0);
                uint32_t shifted_crN;
                if (i == 7) {
                    shifted_crN = crN;
                } else {
                    DECLARE_NEW_REGISTER(shift_count, RTLTYPE_INT32);
                    ADD_INSN(RTLOP_LOAD_IMM, shift_count, 0, 0, (7 - i) * 4);
                    ALLOC_REGISTER(shifted_crN, RTLTYPE_INT32);
                    ADD_INSN(RTLOP_SLL, shifted_crN, crN, shift_count, 0);
                }
                DECLARE_NEW_REGISTER(new_cr, RTLTYPE_INT32);
                ADD_INSN(RTLOP_OR, new_cr, cr, shifted_crN, 0);
                cr = new_cr;
            }
        }
        ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, cr, 0,
                 ctx->handle->setup.state_offset_cr);
    }

    if (ctx->lr_changed) {
        DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, reg, ctx->alias.lr, 0, 0);
        ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, reg, 0,
                 ctx->handle->setup.state_offset_lr);
    }

    if (ctx->ctr_changed) {
        DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, reg, ctx->alias.ctr, 0, 0);
        ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, reg, 0,
                 ctx->handle->setup.state_offset_ctr);
    }

    if (ctx->xer_changed) {
        DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, reg, ctx->alias.xer, 0, 0);
        ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, reg, 0,
                 ctx->handle->setup.state_offset_xer);
    }

    if (ctx->fpscr_changed) {
        DECLARE_NEW_REGISTER(reg, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, reg, ctx->alias.fpscr, 0, 0);
        ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, reg, 0,
                 ctx->handle->setup.state_offset_fpscr);
    }

    if (ctx->reserve_changed) {
        DECLARE_NEW_REGISTER(flag, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, flag, ctx->alias.reserve_flag, 0, 0);
        ADD_INSN(RTLOP_STORE_I8, ctx->psb_reg, flag, 0,
                 ctx->handle->setup.state_offset_reserve_flag);
        DECLARE_NEW_REGISTER(address, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, address, ctx->alias.reserve_address, 0, 0);
        ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, address, 0,
                 ctx->handle->setup.state_offset_reserve_address);
    }

    DECLARE_NEW_REGISTER(nia_reg, RTLTYPE_INT32);
    ADD_INSN(RTLOP_GET_ALIAS, nia_reg, ctx->alias.nia, 0, 0);
    ADD_INSN(RTLOP_STORE_I32, ctx->psb_reg, nia_reg, 0,
             ctx->handle->setup.state_offset_nia);
    ADD_INSN(RTLOP_RETURN, 0, ctx->psb_reg, 0, 0);

    return true;
}

/*************************************************************************/
/************************ Translation entry point ************************/
/*************************************************************************/

bool guest_ppc_translate(binrec_t *handle, uint32_t address, RTLUnit *unit)
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

    GuestPPCContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.blocks = NULL;

    ctx.handle = handle;
    ctx.unit = unit;
    ctx.start = address;

    /* Scan guest memory to determine the range of code to translate and
     * record relevant properties about the code. */
    if (!guest_ppc_scan(&ctx)) {
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
        if (!guest_ppc_gen_rtl(&ctx, i)) {
            goto error;
        }
    }

    /* Append the epilogue, storing all modified registers back to the
     * processor state block and returning to the caller. */
    if (!add_epilogue(&ctx)) {
        goto error;
    }

    free(ctx.blocks);
    return 1;

  error:
    free(ctx.blocks);
    return 0;
}

/*************************************************************************/
/*************************************************************************/
