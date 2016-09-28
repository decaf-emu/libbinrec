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
/*************************** Utility routines ****************************/
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
        ASSERT(ctx->alias.gpr[index]);
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
        ASSERT(ctx->alias.fpr[index]);
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
        ASSERT(ctx->alias.cr[index]);
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
        ASSERT(ctx->alias.lr);
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
        ASSERT(ctx->alias.ctr);
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
        ASSERT(ctx->alias.xer);
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
        ASSERT(ctx->alias.fpscr);
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
 * set_nia_reg:  Set the NIA field of the processor state block to the
 * value of the given RTL register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register containing value to store.
 */
static inline void set_nia_reg(GuestPPCContext *ctx, int reg)
{
    rtl_add_insn(ctx->unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.nia);
}

/*-----------------------------------------------------------------------*/

/**
 * set_nia_imm:  Set the NIA field of the processor state block to the
 * given literal value.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     nia: Address to store in the state block's NIA field.
 */
static inline void set_nia_imm(GuestPPCContext *ctx, uint32_t nia)
{
    RTLUnit * const unit = ctx->unit;

    const int nia_reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, nia_reg, 0, 0, nia);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, nia_reg, 0, ctx->alias.nia);
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

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTSI, lt, result, 0, 0);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTSI, gt, result, 0, 0);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, eq, result, 0, 0);
    const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, so, get_xer(ctx), 0, 31 | 1<<8);

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

/*************************************************************************/
/*************************** Translation core ****************************/
/*************************************************************************/

/**
 * translate_arith_imm:  Translate an integer register-immediate arithmetic
 * instruction.  For addi and addis, rA is assumed to be nonzero.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     shift_imm: True if the immediate value should be shifted left 16 bits.
 *     set_ca: True if XER[CA] should be set according to the result.
 *     set_cr0: True if CR0 should be set according to the result.
 */
static inline void translate_arith_imm(
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
 * translate_branch_label:  Translate a branch to another block within the
 * current unit.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     target_label: RTL label for the target block.
 */
static void translate_branch_label(
    GuestPPCContext *ctx, GuestPPCBlockInfo *block, uint32_t address,
    int BO, int BI, int target_label)
{
    RTLUnit * const unit = ctx->unit;

    int skip_label;
    if ((BO & 0x14) == 0) {
        /* Need an extra label in case the first half of the test fails. */
        skip_label = rtl_alloc_label(unit);
    } else {
        skip_label = 0;
    }

    RTLOpcode branch_op = RTLOP_GOTO;
    int test_reg = 0;

    if (!(BO & 0x04)) {
        const int ctr = get_ctr(ctx);
        const int new_ctr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, new_ctr, ctr, 0, -1);
        set_ctr(ctx, new_ctr);
        if (skip_label) {
            rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                         0, new_ctr, 0, skip_label);
        } else {
            branch_op = BO & 0x02 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ;
            test_reg = new_ctr;
        }
    }

    if (!(BO & 0x10)) {
        const int crN = get_cr(ctx, BI >> 2);
        const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, test, crN, 0, 1 << (3 - (BI & 3)));
        branch_op = BO & 0x08 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z;
        test_reg = test;
    }

    store_live_regs(ctx, block);
    rtl_add_insn(unit, branch_op, 0, test_reg, 0, target_label);

    if (skip_label) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_branch_terminal:  Translate a branch which terminates
 * execution of the current unit.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     LK: LK bit of instruction word.
 *     target: RTL register containing the branch target address.
 *     clear_low_bits: True to clear the low 2 bits of the target address
 *         (as for BCLR/BCCTR), false if the low bits are known to be 0b00.
 */
static void translate_branch_terminal(
    GuestPPCContext *ctx, GuestPPCBlockInfo *block, uint32_t address,
    int BO, int BI, int LK, int target, bool clear_low_bits)
{
    RTLUnit * const unit = ctx->unit;

    if (clear_low_bits) {
        const int new_target = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, new_target, target, 0, -4);
        target = new_target;
    }

    int skip_label;
    if ((BO & 0x14) == 0x14) {
        skip_label = 0;  // Unconditional branch.
    } else {
        skip_label = rtl_alloc_label(unit);
    }

    if (!(BO & 0x04)) {
        const int ctr = get_ctr(ctx);
        const int new_ctr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, new_ctr, ctr, 0, -1);
        set_ctr(ctx, new_ctr);
        rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                     0, new_ctr, 0, skip_label);
    }

    if (!(BO & 0x10)) {
        const int crN = get_cr(ctx, BI >> 2);
        const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, test, crN, 0, 1 << (3 - (BI & 3)));
        rtl_add_insn(unit, BO & 0x08 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ,
                     0, test, 0, skip_label);
    }

    /* Save the current value of LR before (possibly) modifying it so we
     * don't leak the modified LR to the not-taken code path. */
    int current_lr = ctx->live.lr;
    if (LK) {
        const int new_lr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, new_lr, 0, 0, address + 4);
        set_lr(ctx, new_lr);
    }
    store_live_regs(ctx, block);
    ctx->live.lr = current_lr;
    set_nia_reg(ctx, target);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, ctx->epilogue_label);

    if (skip_label) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_branch:  Translate an immediate-displacement branch instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     disp: Displacement field of instruction word.
 *     AA: AA bit of instruction word.
 *     LK: LK bit of instruction word.
 */
static inline void translate_branch(
    GuestPPCContext *ctx, GuestPPCBlockInfo *block, uint32_t address,
    int BO, int BI, int32_t disp, int AA, int LK)
{
    ASSERT((address & 3) == 0);
    ASSERT((disp & 3) == 0);

    RTLUnit * const unit = ctx->unit;

    uint32_t target;
    if (AA) {
        target = (uint32_t)disp;
    } else {
        target = address + disp;
    }

    int target_label = 0;
    if (!LK) {
        ASSERT(ctx->num_blocks > 0);
        const uint32_t unit_start = ctx->blocks[0].start;
        const uint32_t unit_end = (ctx->blocks[ctx->num_blocks-1].start
                                   + ctx->blocks[ctx->num_blocks-1].len - 1);
        if (target >= unit_start && target <= unit_end) {
            int lo = 0, hi = ctx->num_blocks - 1;
            /* We break blocks at all branch targets, so this search should
             * always terminate at a match. */
            while (!target_label) {
                ASSERT(lo <= hi);
                const int mid = (lo + hi + 1) / 2;
                const GuestPPCBlockInfo *mid_block = &ctx->blocks[mid];
                const uint32_t mid_start = mid_block->start;
                if (target == mid_start) {
                    target_label = mid_block->label;
                } else if (target < mid_start) {
                    hi = mid - 1;
                } else {
                    /* The target should never be in the middle of a block,
                     * since we split blocks at branch targets during
                     * scanning. */
                    ASSERT(target > mid_start + mid_block->len - 1);
                    lo = mid + 1;
                }
            }
        }
    }

    if (target_label) {
        translate_branch_label(ctx, block, address, BO, BI, target_label);
    } else {
        const int target_reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, target_reg, 0, 0, target);
        translate_branch_terminal(ctx, block, address, BO, BI, LK,
                                  target_reg, false);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_compare_imm:  Translate an integer register-immediate comapre
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_signed: True for a signed compare, false for an unsigned compare.
 */
static inline void translate_compare_imm(
    GuestPPCContext *ctx, uint32_t insn, bool is_signed)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, get_rA(insn));
    const int32_t imm = is_signed ? get_SIMM(insn) : (int32_t)get_UIMM(insn);

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, is_signed ? RTLOP_SLTSI : RTLOP_SLTUI, lt, rA, 0, imm);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, is_signed ? RTLOP_SGTSI : RTLOP_SGTUI, gt, rA, 0, imm);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, eq, rA, 0, imm);
    const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, so, get_xer(ctx), 0, 31 | 1<<8);

    const int eq_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, eq_shifted, eq, 0, 1);
    const int crN_2 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, crN_2, so, eq_shifted, 0);
    const int gt_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, gt_shifted, gt, 0, 2);
    const int crN_3 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, crN_3, crN_2, gt_shifted, 0);
    const int lt_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, lt_shifted, lt, 0, 3);
    const int crN = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, crN, crN_3, lt_shifted, 0);

    set_cr(ctx, get_crfD(insn), crN);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_logic_imm:  Translate an integer register-immediate logic
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     shift_imm: True if the immediate value should be shifted left 16 bits.
 *     set_cr0: True if CR0 should be set according to the result.
 */
static inline void translate_logic_imm(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool shift_imm,
    bool set_cr0)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, get_rS(insn));
    const uint32_t imm = shift_imm ? get_UIMM(insn)<<16 : get_UIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, 0, (int32_t)imm);
    set_gpr(ctx, get_rA(insn), result);

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
    set_nia_imm(ctx, address);
    guest_ppc_flush_state(ctx);
    const int trap_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD, trap_handler, ctx->psb_reg, 0,
                 ctx->handle->setup.state_offset_trap_handler);
    rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, trap_handler, ctx->psb_reg, 0);
    rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);

    if (result) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_unimplemented_insn:  Handle translation for an instruction
 * not supported by the translator.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static void translate_unimplemented_insn(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn)
{
    log_warning(ctx->handle, "Unsupported instruction %08X at address 0x%X,"
                " treating as invalid", insn, address);
    rtl_add_insn(ctx->unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
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
        rtl_add_insn(unit, RTLOP_LOAD_IMM, imm, 0, 0,
                     (uint32_t)get_SIMM(insn));
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

      case OPCD_CMPLI:
        translate_compare_imm(ctx, insn, false);
        return;

      case OPCD_CMPI:
        translate_compare_imm(ctx, insn, true);
        return;

      case OPCD_ADDIC:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, false);
        return;

      case OPCD_ADDIC_:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, true);
        return;

      case OPCD_ADDI:
        if (get_rA(insn) == 0) {  // li
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_IMM,
                         result, 0, 0, (uint32_t)get_SIMM(insn));
            set_gpr(ctx, get_rD(insn), result);
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, false, false, false);
        }
        return;

      case OPCD_ADDIS:
        if (get_rA(insn) == 0) {  // lis
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_IMM,
                         result, 0, 0, (uint32_t)get_SIMM(insn) << 16);
            set_gpr(ctx, get_rD(insn), result);
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, true, false, false);
        }
        return;

      case OPCD_BC:
        translate_branch(ctx, block, address, get_BO(insn), get_BI(insn),
                         get_BD(insn), get_AA(insn), get_LK(insn));
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
            set_nia_reg(ctx, get_lr(ctx));
        } else {
            set_nia_imm(ctx, address + 4);
        }
        guest_ppc_flush_state(ctx);
        const int sc_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD, sc_handler, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_sc_handler);
        rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, sc_handler, ctx->psb_reg, 0);
        rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);
        return;
      }  // case OPCD_SC

      case OPCD_B:
        translate_branch(ctx, block, address, 0x14, 0, get_LI(insn),
                         get_AA(insn), get_LK(insn));
        return;

      case OPCD_x13:
        switch ((PPCExtendedOpcode13)get_XO_10(insn)) {
          case XO_BCLR:
            translate_branch_terminal(ctx, block, address, get_BO(insn),
                                      get_BI(insn), get_LK(insn),
                                      get_lr(ctx), true);
            return;

          case XO_RFI:  // rfi
            translate_unimplemented_insn(ctx, address, insn);
            return;

          case XO_BCCTR:
            translate_branch_terminal(ctx, block, address, get_BO(insn),
                                      get_BI(insn), get_LK(insn),
                                      get_ctr(ctx), true);
            return;

          default:
            return;  // FIXME: not yet implemented
        }
        ASSERT(!"Missing 0x13 extended opcode handler");

      case OPCD_ORI:
        if (insn == 0x60000000) {  // nop
            return;
        }
        translate_logic_imm(ctx, insn, RTLOP_ORI, false, false);
        return;

      case OPCD_ORIS:
        translate_logic_imm(ctx, insn, RTLOP_ORI, true, false);
        return;

      case OPCD_XORI:
        translate_logic_imm(ctx, insn, RTLOP_XORI, false, false);
        return;

      case OPCD_XORIS:
        translate_logic_imm(ctx, insn, RTLOP_XORI, true, false);
        return;

      case OPCD_ANDI_:
        translate_logic_imm(ctx, insn, RTLOP_ANDI, false, true);
        return;

      case OPCD_ANDIS_:
        translate_logic_imm(ctx, insn, RTLOP_ANDI, true, true);
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
    bool last_was_sc = false;
    for (uint32_t ofs = 0; ofs < block->len; ofs += 4) {
        const uint32_t address = start + ofs;
        const uint32_t insn = bswap_be32(memory_base[address/4]);
        if (last_was_sc && insn == 0x4E800020) {  // blr
            /* Special case optimized by the sc translator. */
            last_was_sc = false;
            continue;
        }
        last_was_sc = (get_OPCD(insn) == OPCD_SC);
        translate_insn(ctx, block, address, insn);
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate instruction at 0x%X",
                    address);
            return false;
        }
    }

    store_live_regs(ctx, block);
    set_nia_imm(ctx, start + block->len);
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
