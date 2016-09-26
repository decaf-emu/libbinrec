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
 * update_cr0:  Add RTL instructions to update the value of CR0 based on
 * the result of an integer operation.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     result: RTL register containing operation result.
 */
static void update_cr0(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const int result)
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

    const int cr0_2 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFINS, cr0_2, so, eq, 1 | 1<<8);
    const int cr0_3 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFINS, cr0_3, cr0_2, gt, 2 | 1<<8);
    const int cr0 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFINS, cr0, cr0_3, lt, 3 | 1<<8);
    set_cr(ctx, 0, cr0);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_arith_imm:  Translate an integer register-immediate arithmetic
 * instruction.  For addi, rA is assumed to be nonzero.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     shift_imm: True if the immediate value should be shifted left 16 bits.
 *     set_ca: True if XER[CA] should be set according to the result.
 *     set_cr0: True if CR0 should be set according to the result.
 */
static void translate_arith_imm(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn, const RTLOpcode rtlop,
    const bool shift_imm, const bool set_ca, const bool set_cr0)
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
        update_cr0(ctx, block, address, result);
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
      case OPCD_MULLI:
        translate_arith_imm(ctx, block, address, insn,
                            RTLOP_MULI, false, false, false);
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
      }

      case OPCD_ADDIC:
        translate_arith_imm(ctx, block, address, insn,
                            RTLOP_ADDI, false, true, false);
        return;

      case OPCD_ADDIC_:
        translate_arith_imm(ctx, block, address, insn,
                            RTLOP_ADDI, false, true, true);
        return;

      case OPCD_ADDI:
        if (get_rA(insn) == 0) {
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD_IMM,
                         result, 0, 0, (uint32_t)get_SIMM(insn));
            set_gpr(ctx, get_rD(insn), result);
        } else {
            translate_arith_imm(ctx, block, address, insn,
                                RTLOP_ADDI, false, false, false);
        }
        return;

      case OPCD_ADDIS:
        translate_arith_imm(ctx, block, address, insn,
                            RTLOP_ADDI, true, false, false);
        return;

      default: return;  // FIXME: not yet implemented
    }  // switch (get_OPCD(insn))

    UNREACHABLE;
}

/*-----------------------------------------------------------------------*/

/**
 * store_live_regs:  Copy live shadow registers at the end of a block to
 * their respective aliases.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Block which was just translated.
 */
static void store_live_regs(GuestPPCContext *ctx,
                            const GuestPPCBlockInfo *block)
{
    RTLUnit * const unit = ctx->unit;

    for (uint32_t gpr_changed = block->gpr_changed; gpr_changed; ) {
        const int index = ctz32(gpr_changed);
        gpr_changed ^= 1u << index;
        // FIXME: This should be an assertion, but it won't hold until we implement all instructions.  Similarly below.
        if (!ctx->live.gpr[index]) continue;  // FIXME: ASSERT(ctx->live.gpr[index]);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.gpr[index], 0, ctx->alias.gpr[index]);
    }

    for (uint32_t fpr_changed = block->fpr_changed; fpr_changed; ) {
        const int index = ctz32(fpr_changed);
        fpr_changed ^= 1u << index;
        if (!ctx->live.fpr[index]) continue;  // FIXME: ASSERT(ctx->live.fpr[index]);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.fpr[index], 0, ctx->alias.fpr[index]);
    }

    for (uint32_t cr_changed = block->cr_changed; cr_changed; ) {
        const int index = ctz32(cr_changed);
        cr_changed ^= 1u << index;
        if (!ctx->live.cr[index]) continue;  // FIXME: ASSERT(ctx->live.cr[index]);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.cr[index], 0, ctx->alias.cr[index]);
    }

    if (block->lr_changed) {
        if (ctx->live.lr)  // FIXME: ASSERT(ctx->live.lr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.lr, 0, ctx->alias.lr);
    }

    if (block->ctr_changed) {
        if (ctx->live.ctr)  // FIXME: ASSERT(ctx->live.ctr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.ctr, 0, ctx->alias.ctr);
    }

    if (block->xer_changed) {
        if (ctx->live.xer)  // FIXME: ASSERT(ctx->live.xer);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.xer, 0, ctx->alias.xer);
    }

    if (block->fpscr_changed) {
        if (ctx->live.fpscr)  // FIXME: ASSERT(ctx->live.fpscr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.fpscr, 0, ctx->alias.fpscr);
    }

    /* reserve_flag and reserve_address are written directly when used. */
}

/*************************************************************************/
/************************ Translation entry point ************************/
/*************************************************************************/

bool guest_ppc_translate_block(GuestPPCContext *ctx, int index)
{
    ASSERT(ctx);
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
    const int nia_reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, nia_reg, 0, 0, start + block->len);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, nia_reg, 0, ctx->alias.nia);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to update registers after block end 0x%X",
                start + block->len - 4);
        return false;
    }

    return true;
}

/*************************************************************************/
/*************************************************************************/
