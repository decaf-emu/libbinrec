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
#include "src/endian.h"
#include "src/guest-ppc/guest-ppc-decode.h"
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl.h"

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * get_gpr, get_fpr, get_cr, get_lr, get_ctr, get_xer, get_fpscr:  Return
 * an RTL register containing the value of the given PowerPC register.
 * This will either be the register last used in a corresponding set or get
 * operation, or a newly allocated register (in which case an appropriate
 * GET_ALIAS instruction will also be added).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (get_gpr(), get_fpr(), get_cr() only).
 * [Return value]
 *     RTL register index.
 */
static inline int get_gpr(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.gpr[index]) {
        return ctx->live.gpr[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.gpr[index]);
        ctx->live.gpr[index] = reg;
        return reg;
    }
}

static inline int get_fpr(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.fpr[index]) {
        return ctx->live.fpr[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_V2_DOUBLE);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fpr[index]);
        ctx->live.fpr[index] = reg;
        return reg;
    }
}

static inline int get_cr(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.cr[index]) {
        return ctx->live.cr[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.cr[index]);
        ctx->live.cr[index] = reg;
        return reg;
    }
}

static inline int get_lr(GuestPPCContext * const ctx)
{
    if (ctx->live.lr) {
        return ctx->live.lr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.lr);
        ctx->live.lr = reg;
        return reg;
    }
}

static inline int get_ctr(GuestPPCContext * const ctx)
{
    if (ctx->live.ctr) {
        return ctx->live.ctr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.ctr);
        ctx->live.ctr = reg;
        return reg;
    }
}

static inline int get_xer(GuestPPCContext * const ctx)
{
    if (ctx->live.xer) {
        return ctx->live.xer;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.xer);
        ctx->live.xer = reg;
        return reg;
    }
}

static inline int get_fpscr(GuestPPCContext * const ctx)
{
    if (ctx->live.fpscr) {
        return ctx->live.fpscr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fpscr);
        ctx->live.fpscr = reg;
        return reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_gpr, set_fpr, set_cr, set_lr, set_ctr, set_xer, set_fpscr:  Store
 * the given RTL register to the given PowerPC register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (get_gpr(), get_fpr(), get_cr() only).
 *     reg: Register to store.
 */
static inline void set_gpr(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.gpr[index] = reg;
}

static inline void set_fpr(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.fpr[index] = reg;
}

static inline void set_cr(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.cr[index] = reg;
}

static inline void set_lr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.lr = reg;
}

static inline void set_ctr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.ctr = reg;
}

static inline void set_xer(GuestPPCContext * const ctx, int reg)
{
    ctx->live.xer = reg;
}

static inline void set_fpscr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.fpscr = reg;
}

/*-----------------------------------------------------------------------*/

/**
 * store_live_regs:  Copy live shadow registers to their respective aliases.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Block which was just translated.
 */
static void store_live_regs(GuestPPCContext *ctx,
                            const GuestPPCBlockInfo *block)
{
    RTLUnit * const unit = ctx->unit;

    /* We use *_changed here as a hint to speed up the search for live
     * registers: any register without a set bit can never be dirty, so
     * we don't need to look at it. */
    for (uint32_t gpr_changed = block->gpr_changed; gpr_changed; ) {
        const int index = ctz32(gpr_changed);
        gpr_changed ^= 1u << index;
        if (ctx->live.gpr[index]) {
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.gpr[index], 0, ctx->alias.gpr[index]);
        }
    }

    for (uint32_t fpr_changed = block->fpr_changed; fpr_changed; ) {
        const int index = ctz32(fpr_changed);
        fpr_changed ^= 1u << index;
        if (ctx->live.fpr[index]) {
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.fpr[index], 0, ctx->alias.fpr[index]);
        }
    }

    for (uint32_t cr_changed = block->cr_changed; cr_changed; ) {
        const int index = ctz32(cr_changed);
        cr_changed ^= 1u << index;
        if (ctx->live.cr[index]) {
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.cr[index], 0, ctx->alias.cr[index]);
        }
    }

    if (block->lr_changed && ctx->live.lr) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.lr, 0, ctx->alias.lr);
    }

    if (block->ctr_changed && ctx->live.ctr) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.ctr, 0, ctx->alias.ctr);
    }

    if (block->xer_changed && ctx->live.xer) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.xer, 0, ctx->alias.xer);
    }

    if (block->fpscr_changed && ctx->live.fpscr) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.fpscr, 0, ctx->alias.fpscr);
    }

    /* reserve_flag and reserve_address are set directly when used. */
}

/*-----------------------------------------------------------------------*/

/**
 * set_nia:  Update the NIA field of the processor state block.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     nia: Address to store in the state block's NIA field.
 */
static void set_nia(GuestPPCContext *ctx, uint32_t nia)
{
    RTLUnit * const unit = ctx->unit;

    const int nia_reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, nia_reg, 0, 0, nia);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, nia_reg, 0, ctx->alias.nia);
}

/*-----------------------------------------------------------------------*/

/**
 * set_nia_lr:  Set the NIA field of the processor state block to the
 * current value of the LR register.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
static void set_nia_lr(GuestPPCContext *ctx)
{
    rtl_add_insn(ctx->unit, RTLOP_SET_ALIAS,
                 0, get_lr(ctx), 0, ctx->alias.nia);
}

/*-----------------------------------------------------------------------*/

/**
 * update_cr0:  Add RTL instructions to update the value of CR0 based on
 * the result of an integer operation.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     result: RTL register containing operation result.
 */
static void update_cr0(GuestPPCContext *ctx, int result)
{
    RTLUnit * const unit = ctx->unit;

    const int xer = get_xer(ctx);

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTSI, lt, result, 0, 0);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTSI, gt, result, 0, 0);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, eq, result, 0, 0);
    const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, so, xer, 0, 31 | 1<<8);

    const int eq_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, eq_shifted, eq, 0, 1);
    const int cr0_2 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, cr0_2, so, eq_shifted, 0);
    const int gt_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, gt_shifted, gt, 0, 2);
    const int cr0_3 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, cr0_3, cr0_2, gt_shifted, 0);
    const int lt_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, lt_shifted, lt, 0, 3);
    const int cr0 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, cr0, cr0_3, lt_shifted, 0);

    set_cr(ctx, 0, cr0);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_arith_imm:  Translate an integer register-immediate arithmetic
 * instruction.  For addi, rA is assumed to be nonzero.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     shift_imm: True if the immediate value should be shifted left 16 bits.
 *     set_ca: True if XER[CA] should be set according to the result.
 *     set_cr0: True if CR0 should be set according to the result.
 */
static void translate_arith_imm(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool shift_imm,
    bool set_ca, bool set_cr0)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, get_rA(insn));
    const int32_t imm = shift_imm ? get_SIMM(insn)<<16 : get_SIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rA, 0, imm);
    set_gpr(ctx, get_rD(insn), result);

    if (set_ca) {
        ASSERT(rtlop == RTLOP_ADDI);
        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTUI, ca, result, 0, imm);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, 29 | 1<<8);
        set_xer(ctx, new_xer);
    }

    if (set_cr0) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_trap:  Translate a TW or TWI instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 *     rB: RTL register containing second comparison value (rB or immediate).
 */
static void translate_trap(
    GuestPPCContext *ctx, GuestPPCBlockInfo *block, uint32_t address,
    uint32_t insn, int rB)
{
    RTLUnit * const unit = ctx->unit;

    const int TO = get_TO(insn);
    if (!TO) {
        return;  // Effectively a NOP.
    }

    const int label = rtl_alloc_label(unit);
    const int rA = get_gpr(ctx, get_rA(insn));
    int result;
    RTLOpcode skip_op;

    switch (TO) {
      case TO_GTU:                              // 0x01
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_LTU:                              // 0x02
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_LTU | TO_GTU:                     // 0x03
      case TO_GTS | TO_LTU | TO_GTU:            // 0x0B
      case TO_LTS | TO_LTU | TO_GTU:            // 0x13
      case TO_LTS | TO_GTS:                     // 0x18
      case TO_LTS | TO_GTS | TO_GTU:            // 0x19
      case TO_LTS | TO_GTS | TO_LTU:            // 0x1A
      case TO_LTS | TO_GTS | TO_LTU | TO_GTU:   // 0x1B
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQ, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_NZ;
        break;

      case TO_EQ:                               // 0x04
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQ, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_GTU | TO_EQ:                      // 0x05
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_NZ;
        break;

      case TO_LTU | TO_EQ:                      // 0x06
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, result, rA, rB, 0);
        skip_op = RTLOP_GOTO_IF_NZ;
        break;

      case TO_GTS:                              // 0x08
      case TO_GTS | TO_GTU:                     // 0x09
      case TO_GTS | TO_LTU:                     // 0x0A
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        } else if (TO & TO_LTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        }
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_GTS | TO_EQ:                      // 0x0C
      case TO_GTS | TO_EQ | TO_GTU:             // 0x0D
      case TO_GTS | TO_EQ | TO_LTU:             // 0x0E
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else if (TO & TO_LTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else {
            skip_op = RTLOP_GOTO_IF_NZ;
        }
        break;

      case TO_LTS:                              // 0x10
      case TO_LTS | TO_GTU:                     // 0x11
      case TO_LTS | TO_LTU:                     // 0x12
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        } else if (TO & TO_LTU) {
            const int result1 = result;
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
        }
        skip_op = RTLOP_GOTO_IF_Z;
        break;

      case TO_LTS | TO_EQ:                      // 0x14
      case TO_LTS | TO_EQ | TO_GTU:             // 0x15
      case TO_LTS | TO_EQ | TO_LTU:             // 0x16
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTS, result, rA, rB, 0);
        if (TO & TO_GTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SGTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else if (TO & TO_LTU) {
            const int result1 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, result1, result, 0, 1);
            const int result2 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLTU, result2, rA, rB, 0);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, result1, result2, 0);
            skip_op = RTLOP_GOTO_IF_Z;
        } else {
            skip_op = RTLOP_GOTO_IF_NZ;
        }
        break;

      default:
        assert((TO & (TO_LTS|TO_GTS|TO_EQ)) == (TO_LTS|TO_GTS|TO_EQ)
            || (TO & (TO_LTU|TO_GTU|TO_EQ)) == (TO_LTU|TO_GTU|TO_EQ));
        result = 0;
        skip_op = RTLOP_NOP;
        break;
    }

    if (result) {
        rtl_add_insn(unit, skip_op, 0, result, 0, label);
    }

    store_live_regs(ctx, block);
    set_nia(ctx, address);
    const int trap_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD, trap_handler, ctx->psb_reg, 0,
                 ctx->handle->setup.state_offset_trap_handler);
    rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, trap_handler, ctx->psb_reg, 0);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, ctx->epilogue_label);

    if (result) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_insn:  Translate the given instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 * [Return value]
 *     True on success, false on error.
 */
static inline void translate_insn(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (UNLIKELY(!is_valid_insn(insn))) {
        rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        return;
    }

    switch (get_OPCD(insn)) {
      case OPCD_TWI: {
        const int imm = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, imm, 0, 0, get_SIMM(insn));
        translate_trap(ctx, block, address, insn, imm);
        return;
      }  // case OPCD_TWI

      case OPCD_MULLI:
        translate_arith_imm(ctx, insn, RTLOP_MULI, false, false, false);
        return;

      case OPCD_SUBFIC: {
        const int rA = get_gpr(ctx, get_rA(insn));
        const int imm = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     imm, 0, 0, (uint32_t)get_SIMM(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, imm, rA, 0);
        set_gpr(ctx, get_rD(insn), result);

        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, ca, imm, result, 0);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, 29 | 1<<8);
        set_xer(ctx, new_xer);

        return;
      }  // case OPCD_SUBFIC

      case OPCD_ADDIC:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, false);
        return;

      case OPCD_ADDIC_:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, true);
        return;

      case OPCD_ADDI:
        if (get_rA(insn) == 0) {
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_IMM,
                         result, 0, 0, (uint32_t)get_SIMM(insn));
            set_gpr(ctx, get_rD(insn), result);
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, false, false, false);
        }
        return;

      case OPCD_ADDIS:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, true, false, false);
        return;

      case OPCD_SC: {
        /* Special case: translate sc followed by blr in a single step, to
         * avoid having to return to caller and call a new unit containing
         * just the blr. */
        bool is_sc_blr = false;
        if (address + 7 <= block->start + block->len - 1) {
            const uint32_t *memory_base =
                (const uint32_t *)ctx->handle->setup.memory_base;
            const uint32_t next_insn = bswap_be32(memory_base[(address+4)/4]);
            if (next_insn == 0x4E800020) {
                is_sc_blr = true;
            }
        }
        store_live_regs(ctx, block);
        if (is_sc_blr) {
            set_nia_lr(ctx);
        } else {
            set_nia(ctx, address + 4);
        }
        const int sc_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD, sc_handler, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_sc_handler);
        rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, sc_handler, ctx->psb_reg, 0);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, ctx->epilogue_label);
        return;
      }  // case OPCD_SC

      case OPCD_ORI:
        if (insn == 0x60000000) {  // nop
            return;
        }
        // FIXME: not yet implemented
        return;

      case OPCD_x1F:
        switch ((PPCExtendedOpcode1F)get_XO_10(insn)) {
          case XO_TW:
            translate_trap(ctx, block, address, insn,
                           get_gpr(ctx, get_rB(insn)));
            return;

          default: return;  // FIXME: not yet implemented
        }
        ASSERT(!"Missing 0x1F extended opcode handler");

      default: return;  // FIXME: not yet implemented
    }  // switch (get_OPCD(insn))

    ASSERT(!"Missing primary opcode handler");
}

/*************************************************************************/
/************************** Exported functions ***************************/
/*************************************************************************/

bool guest_ppc_translate_block(GuestPPCContext *ctx, int index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(index >= 0 && index < ctx->num_blocks);

    RTLUnit * const unit = ctx->unit;
    GuestPPCBlockInfo *block = &ctx->blocks[index];
    const uint32_t start = block->start;
    const uint32_t *memory_base =
        (const uint32_t *)ctx->handle->setup.memory_base;

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, block->label);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to add label at 0x%X", start);
        return false;
    }

    memset(&ctx->live, 0, sizeof(ctx->live));
    for (uint32_t ofs = 0; ofs < block->len; ofs += 4) {
        const uint32_t address = start + ofs;
        const uint32_t insn = bswap_be32(memory_base[address/4]);
        translate_insn(ctx, block, address, insn);
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate instruction at 0x%X",
                    address);
            return false;
        }
    }

    store_live_regs(ctx, block);
    set_nia(ctx, start + block->len);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to update registers after block end 0x%X",
                start + block->len - 4);
        return false;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

void guest_ppc_flush_state(GuestPPCContext *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    RTLUnit * const unit = ctx->unit;

    if (ctx->cr_changed) {
        int cr;
        cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        if (ctx->cr_changed != 0xFF) {
            rtl_add_insn(unit, RTLOP_LOAD, cr, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_cr);
            uint32_t mask = 0;
            for (int i = 0; i < 8; i++) {
                if (!(ctx->cr_changed & (1 << i))) {
                    mask |= 0xF << ((7 - i) * 4);
                }
            }
            const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, new_cr, cr, 0, (int32_t)mask);
            cr = new_cr;
        }
        for (int i = 0; i < 8; i++) {
            if (ctx->cr_changed & (1 << i)) {
                const int crN = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_GET_ALIAS,
                             crN, 0, 0, ctx->alias.cr[i]);
                int shifted_crN;
                if (i == 7) {
                    shifted_crN = crN;
                } else {
                    shifted_crN = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_SLLI,
                                 shifted_crN, crN, 0, (7 - i) * 4);
                }
                const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_OR, new_cr, cr, shifted_crN, 0);
                cr = new_cr;
            }
        }
        rtl_add_insn(unit, RTLOP_STORE, 0, ctx->psb_reg, cr,
                     ctx->handle->setup.state_offset_cr);
    }

    if (ctx->reserve_changed) {
        const int flag = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     flag, 0, 0, ctx->alias.reserve_flag);
        rtl_add_insn(unit, RTLOP_STORE_I8, 0, ctx->psb_reg, flag,
                     ctx->handle->setup.state_offset_reserve_flag);
    }
}

/*************************************************************************/
/*************************************************************************/
