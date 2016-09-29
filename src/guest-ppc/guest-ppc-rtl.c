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
 * rtl_imm:  Allocate and return a new RTL register of INT32 type
 * containing the given immediate value.
 */
static inline int rtl_imm(RTLUnit * const unit, uint32_t value)
{
    const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, value);
    return reg;
}

/*-----------------------------------------------------------------------*/

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
 * the given RTL register to the given PowerPC register.  These functions
 * do not add a SET_ALIAS instruction.
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
 *     block: Block which is being or was just translated.
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
 * merge_cr:  Merge all CRn aliases (and untouched CRn fields from the
 * processor state block) into a 32-bit CR word.
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     RTL register containing merged value of CR.
 */
static int merge_cr(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    int cr = rtl_alloc_register(unit, RTLTYPE_INT32);
    if (ctx->cr_changed != 0xFF) {
        rtl_add_insn(unit, RTLOP_LOAD, cr, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_cr);
        if (ctx->cr_changed != 0) {
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
    }

    for (int i = 0; i < 8; i++) {
        if (ctx->cr_changed & (1 << i)) {
            const int crN = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, crN, 0, 0, ctx->alias.cr[i]);
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

    return cr;
}

/*-----------------------------------------------------------------------*/

/**
 * set_nia:  Set the NIA field of the processor state block to the value
 * of the given RTL register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register containing value to store.
 */
static inline void set_nia(GuestPPCContext *ctx, int reg)
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
    set_nia(ctx, rtl_imm(ctx->unit, nia));
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
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer);
    }

    if (set_cr0) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_addsub_reg:  Translate an integer register-register add or
 * subtract instruction.
 *
 * This function implements addition and subtraction as a three-operand
 * addition operation following the PowerPC documentation, which is
 * relatively slow both to translate and to execute.  Simple operations
 * which set no flags should add appropriate RTL instructions directly
 * rather than calling this function.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     srcB_sel: Selector indicating the third operand to the addition:
 *         0 or -1 (constant), or 1 (use rB).
 *     srcC_sel: Selector indicating the third operand to the addition:
 *         0 or 1 (constant), or -1 to use XER[CA].
 *     invert_rA: True to invert (one's-complement) rA, for a subf operation.
 *     set_ca: True if XER[CA] should be set according to the result.
 */
static void translate_addsub_reg(
    GuestPPCContext *ctx, uint32_t insn, int srcB_sel, int srcC_sel,
    bool invert_rA, bool set_ca)
{
    RTLUnit * const unit = ctx->unit;

    int result = 0;

    int srcA = get_gpr(ctx, get_rA(insn));
    if (invert_rA) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, srcA, 0, 0);
        srcA = inverted;
    }

    int srcB = 0;
    if (srcB_sel > 0) {
        srcB = get_gpr(ctx, get_rB(insn));
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, srcA, srcB, 0);
    } else if (srcB_sel < 0) {
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, result, srcA, 0, -1);
    }

    if (srcC_sel > 0) {
        const int temp = result ? result : srcA;
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, result, temp, 0, 1);
    } else if (srcC_sel < 0) {
        const int temp = result ? result : srcA;
        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFEXT, ca, xer, 0, XER_CA_SHIFT | 1<<8);
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, temp, ca, 0);
    }

    set_gpr(ctx, get_rD(insn), result);

    /* Extract high bits of values needed below so we don't have to do it
     * twice. */
    const int a = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, a, srcA, 0, 31);
    int b = 0, a_xor_b = 0;
    if (srcB_sel > 0) {
        b = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SRLI, b, srcB, 0, 31);
        a_xor_b = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XOR, a_xor_b, a, b, 0);
    }
    const int r = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, r, result, 0, 31);

    if (set_ca) {
        /* Carry calculation: XER[CA] = (a && b) || ((a != b) && !result)
         * where a, b, and result are the high bit of each value.
         * (Conceptually: carry always occurs if the high bit of both
         * inputs is set; when the high bit of exactly one input is set,
         * carry occurred if the high bit of the result is clear.)
         * We can also treat "a ^ b" as "a | b", since the latter gives
         * the same result, and we do so in the srcB_sel < 0 case. */
        int ca;
        const int nr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, nr, r, 0, 1);
        if (srcB_sel > 0) {
            const int a_and_b = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, a_and_b, a, b, 0);
            const int and_nr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, and_nr, a_xor_b, nr, 0);
            ca = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, ca, a_and_b, and_nr, 0);
        } else if (srcB_sel < 0) {  // b = 1 --> CA = a | !result
            ca = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, ca, a, nr, 0);
        } else {  // b = 0 --> CA = a & !result
            ca = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ca, a, nr, 0);
        }
        const int xer = get_xer(ctx);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer);
    }

    if (get_OE(insn)) {
        /* Overflow calculation: XER[OV] = (a == b) && (a != result)
         * (where a, b, and result are the high bit of each value)
         * which we implement as: !(a ^ b) & (a ^ result) */
        int ov;
        if (srcB_sel > 0) {
            const int n_a_xor_b = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, n_a_xor_b, a_xor_b, 0, 1);
            const int a_xor_r = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XOR, a_xor_r, a, r, 0);
            ov = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ov, n_a_xor_b, a_xor_r, 0);
        } else if (srcB_sel < 0) {  // b = 1 --> OV = a & !result
            const int nr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, nr, r, 0, 1);
            ov = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ov, a, nr, 0);
        } else {  // b = 0 --> OV = !a & result
            const int na = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XORI, na, a, 0, 1);
            ov = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND, ov, na, r, 0);
        }
        const int xer = get_xer(ctx);
        const int masked_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, (int32_t)~XER_OV);
        const int SO_OV = rtl_imm(unit, XER_SO | XER_OV);
        const int bits_to_set = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, bits_to_set, SO_OV, ov, ov);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, new_xer, masked_xer, bits_to_set, 0);
        set_xer(ctx, new_xer);
    }

    if (get_Rc(insn)) {
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
        set_lr(ctx, rtl_imm(unit, address + 4));
    }
    store_live_regs(ctx, block);
    ctx->live.lr = current_lr;
    set_nia(ctx, target);
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
        translate_branch_terminal(ctx, block, address, BO, BI, LK,
                                  rtl_imm(unit, target), false);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_compare:  Translate an integer comapre instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_imm: True for a register-immediate compare (D-form instruction),
 *         false for a register-register compare (X-form instruction).
 *     is_signed: True for a signed compare, false for an unsigned compare.
 */
static inline void translate_compare(
    GuestPPCContext *ctx, uint32_t insn, bool is_imm, bool is_signed)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, get_rA(insn));

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    if (is_imm) {
        const int32_t imm =
            is_signed ? get_SIMM(insn) : (int32_t)get_UIMM(insn);
        rtl_add_insn(unit, is_signed ? RTLOP_SLTSI : RTLOP_SLTUI,
                     lt, rA, 0, imm);
        rtl_add_insn(unit, is_signed ? RTLOP_SGTSI : RTLOP_SGTUI,
                     gt, rA, 0, imm);
        rtl_add_insn(unit, RTLOP_SEQI, eq, rA, 0, imm);
    } else {
        const int32_t rB = get_gpr(ctx, get_rB(insn));
        rtl_add_insn(unit, is_signed ? RTLOP_SLTS : RTLOP_SLTU, lt, rA, rB, 0);
        rtl_add_insn(unit, is_signed ? RTLOP_SGTS : RTLOP_SGTU, gt, rA, rB, 0);
        rtl_add_insn(unit, RTLOP_SEQ, eq, rA, rB, 0);
    }

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
 * translate_logic_reg:  Translate an integer register-register logic
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     invert_rB: True if the value of rB should be inverted.
 *     invert_result: True if the result should be inverted.
 */
static inline void translate_logic_reg(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool invert_rB,
    bool invert_result)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, get_rS(insn));

    int rB = get_gpr(ctx, get_rB(insn));
    if (invert_rB) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, rB, 0, 0);
        rB = inverted;
    }

    int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, rB, 0);
    if (invert_result) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, result, 0, 0);
        result = inverted;
    }
    set_gpr(ctx, get_rA(insn), result);

    if (get_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_move_spr:  Translate an mfspr or mtspr instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 *     to_spr: True for mtspr, false for mfspr.
 */
static inline void translate_move_spr(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn, bool to_spr)
{
    RTLUnit * const unit = ctx->unit;

    const int spr = get_spr(insn);

    switch (spr) {
      case SPR_XER:
        if (to_spr) {
            set_xer(ctx, get_gpr(ctx, get_rS(insn)));
        } else {
            set_gpr(ctx, get_rD(insn), get_xer(ctx));
        }
        break;

      case SPR_LR:
        if (to_spr) {
            set_lr(ctx, get_gpr(ctx, get_rS(insn)));
        } else {
            set_gpr(ctx, get_rD(insn), get_lr(ctx));
        }
        break;

      case SPR_CTR:
        if (to_spr) {
            set_ctr(ctx, get_gpr(ctx, get_rS(insn)));
        } else {
            set_gpr(ctx, get_rD(insn), get_ctr(ctx));
        }
        break;

      case SPR_TBL:
      case SPR_TBU:
        if (to_spr) {
            log_warning(ctx->handle, "0x%X: Invalid attempt to write TB%c",
                        address, spr==SPR_TBU ? 'U' : 'L');
            rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        } else {
            // FIXME: callout to handler not yet implemented
            set_gpr(ctx, get_rD(insn), rtl_imm(unit, 0));
        }
        break;

      case SPR_UGQR(0):
      case SPR_UGQR(1):
      case SPR_UGQR(2):
      case SPR_UGQR(3):
      case SPR_UGQR(4):
      case SPR_UGQR(5):
      case SPR_UGQR(6):
      case SPR_UGQR(7):
        if (to_spr) {
            const int rS = get_gpr(ctx, get_rS(insn));
            rtl_add_insn(unit, RTLOP_STORE, 0, ctx->psb_reg, rS,
                         ctx->handle->setup.state_offset_gqr + 4 * (spr & 7));
            // FIXME: this should stop GQR optimization when that's implemented
        } else {
            const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD, value, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_gqr + 4 * (spr & 7));
            set_gpr(ctx, get_rD(insn), value);
        }
        break;

      default:
        log_warning(ctx->handle, "0x%X: %s on invalid SPR %d", address,
                    to_spr ? "mtspr" : "mfspr/mftb", spr);
        rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        break;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_muldiv_reg:  Translate an integer register-register multiply
 * or divide instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     do_overflow: True to update XER[SO:OV] based on overflow state.
 */
static inline void translate_muldiv_reg(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool do_overflow)
{
    RTLUnit * const unit = ctx->unit;

    const int rA = get_gpr(ctx, get_rA(insn));
    const int rB = get_gpr(ctx, get_rB(insn));

    int div_skip_label = 0;
    int xer = 0;
    if (rtlop == RTLOP_DIVU || rtlop == RTLOP_DIVS) {
        if (do_overflow) {
            xer = get_xer(ctx);
        }
        div_skip_label = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, rB, 0, div_skip_label);
        if (rtlop == RTLOP_DIVS) {
            const int rA_is_80000000 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI,
                         rA_is_80000000, rA, 0, UINT64_C(-0x80000000));
            const int rB_is_FFFFFFFF = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI, rB_is_FFFFFFFF, rB, 0, -1);
            const int is_invalid = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_AND,
                         is_invalid, rA_is_80000000, rB_is_FFFFFFFF, 0);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, is_invalid, 0, div_skip_label);
        }
    }

    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rA, rB, 0);
    set_gpr(ctx, get_rD(insn), result);

    if (do_overflow) {
        if (!xer) {
            xer = get_xer(ctx);
        }
        const int masked_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, (int32_t)~XER_OV);
        if (rtlop == RTLOP_MUL) {
            const int result_hi = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_MULHU, result_hi, rA, rB, 0);
            const int SO_OV = rtl_imm(unit, XER_SO | XER_OV);
            const int bits_to_set = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SELECT,
                         bits_to_set, SO_OV, result_hi, result_hi);
            const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, new_xer, masked_xer, bits_to_set, 0);
            set_xer(ctx, new_xer);
        } else {
            ASSERT(rtlop == RTLOP_DIVU || rtlop == RTLOP_DIVS);
            set_xer(ctx, masked_xer);
        }
    }

    if (div_skip_label) {
        /* The state of the destination register after an overflow error
         * is explicitly undefined in the PowerPC spec, so we don't bother
         * trying to synchronize the live state of rD here.  We do,
         * however, need to synchronize XER if overflow recording is
         * enabled. */

        int div_continue_label = 0;
        if (do_overflow) {
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.xer, 0, ctx->alias.xer);
            ctx->live.xer = 0;
            div_continue_label = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, div_continue_label);
        }

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, div_skip_label);

        if (do_overflow) {
            const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ORI,
                         new_xer, xer, 0, (int32_t)(XER_SO | XER_OV));
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_xer, 0, ctx->alias.xer);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, div_continue_label);
        }
    }

    if (get_Rc(insn)) {
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
        ASSERT((TO & (TO_LTS|TO_GTS|TO_EQ)) == (TO_LTS|TO_GTS|TO_EQ)
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
 * translate_x1F:  Translate the given opcode-0x1F instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 * [Return value]
 *     True on success, false on error.
 */
static inline void translate_x1F(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    switch ((PPCExtendedOpcode1F)get_XO_10(insn)) {

      /* XO_5 = 0x00 */
      case XO_CMP:
        translate_compare(ctx, insn, false, true);
        return;
      case XO_CMPL:
        translate_compare(ctx, insn, false, false);
        return;
      case XO_MCRXR: {
        const int xer = get_xer(ctx);
        const int xer0_3 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SRLI, xer0_3, xer, 0, 28);
        set_cr(ctx, get_crfD(insn), xer0_3);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, new_xer, xer, 0, 0x0FFFFFFF);
        set_xer(ctx, new_xer);
        return;
      }  // case XO_MCRXR

      /* XO_5 = 0x04 */
      case XO_TW:
        translate_trap(ctx, block, address, insn,
                       get_gpr(ctx, get_rB(insn)));
        return;

      /* XO_5 = 0x08 */
      case XO_SUBFC:
      case XO_SUBFCO:
        translate_addsub_reg(ctx, insn, 1, 1, true, true);
        return;
      case XO_SUBF: {
        const int rA = get_gpr(ctx, get_rA(insn));
        const int rB = get_gpr(ctx, get_rB(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, rB, rA, 0);
        set_gpr(ctx, get_rD(insn), result);
        if (get_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_SUBF
      case XO_SUBFO:
        translate_addsub_reg(ctx, insn, 1, 1, true, false);
        return;
      case XO_NEG: {
        const int rA = get_gpr(ctx, get_rA(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NEG, result, rA, 0, 0);
        set_gpr(ctx, get_rD(insn), result);
        if (get_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_NEG
      case XO_NEGO:
        translate_addsub_reg(ctx, insn, 0, 1, true, false);
        return;
      case XO_SUBFE:
      case XO_SUBFEO:
        translate_addsub_reg(ctx, insn, 1, -1, true, true);
        return;
      case XO_SUBFZE:
      case XO_SUBFZEO:
        translate_addsub_reg(ctx, insn, 0, -1, true, true);
        return;
      case XO_SUBFME:
      case XO_SUBFMEO:
        translate_addsub_reg(ctx, insn, -1, -1, true, true);
        return;

      /* XO_5 = 0x0A */
      case XO_ADDC:
      case XO_ADDCO:
        translate_addsub_reg(ctx, insn, 1, 0, false, true);
        return;
      case XO_ADDE:
      case XO_ADDEO:
        translate_addsub_reg(ctx, insn, 1, -1, false, true);
        return;
      case XO_ADDZE:
      case XO_ADDZEO:
        translate_addsub_reg(ctx, insn, 0, -1, false, true);
        return;
      case XO_ADDME:
      case XO_ADDMEO:
        translate_addsub_reg(ctx, insn, -1, -1, false, true);
        return;
      case XO_ADD: {
        const int rA = get_gpr(ctx, get_rA(insn));
        const int rB = get_gpr(ctx, get_rB(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, rA, rB, 0);
        set_gpr(ctx, get_rD(insn), result);
        if (get_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_ADD
      case XO_ADDO:
        translate_addsub_reg(ctx, insn, 1, 0, false, false);
        return;

      /* XO_5 = 0x0B */
      case XO_MULHWU:
        translate_muldiv_reg(ctx, insn, RTLOP_MULHU, false);
        return;
      case XO_MULHW:
        translate_muldiv_reg(ctx, insn, RTLOP_MULHS, false);
        return;
      case XO_MULLW:
        translate_muldiv_reg(ctx, insn, RTLOP_MUL, false);
        return;
      case XO_MULLWO:
        translate_muldiv_reg(ctx, insn, RTLOP_MUL, true);
        return;
      case XO_DIVWU:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVU, false);
        return;
      case XO_DIVWUO:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVU, true);
        return;
      case XO_DIVW:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVS, false);
        return;
      case XO_DIVWO:
        translate_muldiv_reg(ctx, insn, RTLOP_DIVS, true);
        return;

      /* XO_5 = 0x10 */
      case XO_MTCRF: {
        const int rS = get_gpr(ctx, get_rS(insn));
        if (get_CRM(insn) == 0xFF) {
            rtl_add_insn(unit, RTLOP_STORE, 0, ctx->psb_reg, rS,
                         ctx->handle->setup.state_offset_cr);
        }
        for (int i = 0; i < 8; i++) {
            const int bit = 7 - i;
            if ((get_CRM(insn) & (1 << bit)) && ctx->alias.cr[i]) {
                const int crN = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT, crN, rS, 0, (4*bit) | 4<<8);
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, crN, 0, ctx->alias.cr[i]);
                ctx->live.cr[i] = 0;
            }
        }
        return;
      }  // case XO_MTCRF

      /* XO_5 = 0x12 */
      case XO_MTMSR:
      case XO_MTSR:
      case XO_MTSRIN:
      case XO_TLBIE:
        translate_unimplemented_insn(ctx, address, insn);
        return;

      /* XO_5 = 0x13 */
      case XO_MFCR:
        set_gpr(ctx, get_rD(insn), merge_cr(ctx));
        return;
      case XO_MFTB:
      case XO_MFSPR:
        translate_move_spr(ctx, address, insn, false);
        return;
      case XO_MTSPR:
        translate_move_spr(ctx, address, insn, true);
        return;
      case XO_MFMSR:
      case XO_MFSR:
      case XO_MFSRIN:
        translate_unimplemented_insn(ctx, address, insn);
        return;

      /* XO_5 = 0x1C */
      case XO_AND:
        translate_logic_reg(ctx, insn, RTLOP_AND, false, false);
        return;
      case XO_ANDC:
        translate_logic_reg(ctx, insn, RTLOP_AND, true, false);
        return;
      case XO_NOR:
        translate_logic_reg(ctx, insn, RTLOP_OR, false, true);
        return;
      case XO_EQV:
        /* The PowerPC spec describes this as ~(rS ^ rB), but we implement
         * it as (rS ^ ~rB) since that allows the NOT operation to be
         * scheduled earlier if rB is already loaded. */
        translate_logic_reg(ctx, insn, RTLOP_XOR, true, false);
        return;
      case XO_XOR:
        translate_logic_reg(ctx, insn, RTLOP_XOR, false, false);
        return;
      case XO_ORC:
        translate_logic_reg(ctx, insn, RTLOP_OR, true, false);
        return;
      case XO_OR:
        if (get_rB(insn) == get_rS(insn)) {  // mr rA,rS
            const int rS = get_gpr(ctx, get_rS(insn));
            set_gpr(ctx, get_rA(insn), rS);
            if (get_Rc(insn)) {
                update_cr0(ctx, rS);
            }
        } else {
            translate_logic_reg(ctx, insn, RTLOP_OR, false, false);
        }
        return;
      case XO_NAND:
        translate_logic_reg(ctx, insn, RTLOP_AND, false, true);
        return;

      default: return;  // FIXME: not yet implemented
    }

    ASSERT(!"Missing 0x1F extended opcode handler");
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
      case OPCD_TWI:
        translate_trap(ctx, block, address, insn,
                       rtl_imm(unit, get_SIMM(insn)));
        return;

      case OPCD_MULLI:
        translate_arith_imm(ctx, insn, RTLOP_MULI, false, false, false);
        return;

      case OPCD_SUBFIC: {
        const int rA = get_gpr(ctx, get_rA(insn));
        const int imm = rtl_imm(unit, get_SIMM(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, imm, rA, 0);
        set_gpr(ctx, get_rD(insn), result);

        const int xer = get_xer(ctx);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, ca, imm, result, 0);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer);

        return;
      }  // case OPCD_SUBFIC

      case OPCD_CMPLI:
        translate_compare(ctx, insn, true, false);
        return;

      case OPCD_CMPI:
        translate_compare(ctx, insn, true, true);
        return;

      case OPCD_ADDIC:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, false);
        return;

      case OPCD_ADDIC_:
        translate_arith_imm(ctx, insn, RTLOP_ADDI, false, true, true);
        return;

      case OPCD_ADDI:
        if (get_rA(insn) == 0) {  // li
            set_gpr(ctx, get_rD(insn), rtl_imm(unit, get_SIMM(insn)));
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, false, false, false);
        }
        return;

      case OPCD_ADDIS:
        if (get_rA(insn) == 0) {  // lis
            set_gpr(ctx, get_rD(insn), rtl_imm(unit, get_SIMM(insn) << 16));
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
            set_nia(ctx, get_lr(ctx));
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
        translate_x1F(ctx, block, address, insn);
        return;

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
        const int cr = merge_cr(ctx);
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
