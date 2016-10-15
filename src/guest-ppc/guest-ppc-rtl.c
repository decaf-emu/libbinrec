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
 * rtl_imm32:  Allocate and return a new RTL register of INT32 type
 * containing the given immediate value.
 */
static inline int rtl_imm32(RTLUnit * const unit, uint32_t value)
{
    const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, value);
    return reg;
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_imm64:  Allocate and return a new RTL register of INT64 type
 * containing the given immediate value.
 */
static inline int rtl_imm64(RTLUnit * const unit, uint32_t value)
{
    const int reg = rtl_alloc_register(unit, RTLTYPE_INT64);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, value);
    return reg;
}

/*-----------------------------------------------------------------------*/

/**
 * get_insn_at:  Return the instruction word at the given address, or zero
 * if the given address is outside the current block.
 */
static inline PURE_FUNCTION uint32_t get_insn_at(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address)
{
    if (address >= block->start
     && address + 3 <= block->start + block->len - 1) {
        const uint32_t *memory_base =
            (const uint32_t *)ctx->handle->setup.guest_memory_base;
        return bswap_be32(memory_base[address/4]);
    } else {
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_gpr, get_fpr, get_cr, get_crb, get_lr, get_ctr, get_xer, get_fpscr:
 * Return an RTL register containing the value of the given PowerPC register.
 * This will either be the register last used in a corresponding set or get
 * operation, or a newly allocated register (in which case an appropriate
 * GET_ALIAS instruction will also be added).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (get_gpr(), get_fpr()) or CR bit index
 *         (get_crb()).
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
        const int reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
        ASSERT(ctx->alias.fpr[index]);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fpr[index]);
        ctx->live.fpr[index] = reg;
        return reg;
    }
}

static inline int get_cr(GuestPPCContext * const ctx)
{
    if (ctx->live.cr) {
        return ctx->live.cr;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.cr);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.cr);
        ctx->live.cr = reg;
        return reg;
    }
}

static inline int get_crb(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.crb[index]) {
        return ctx->live.crb[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        int reg;
        ASSERT(ctx->alias.crb[index]);
        if (ctx->crb_loaded & (0x80000000u >> index)) {
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.crb[index]);
        } else {
            ctx->crb_loaded |= 0x80000000u >> index;
            const int cr = get_cr(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT, reg, cr, 0, (31-index) | (1<<8));
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, reg, 0, ctx->alias.crb[index]);
        }
        ctx->live.crb[index] = reg;
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
 * test_crb:  Return an RTL register containing a value which is nonzero if
 * the given CR bit is set and zero if it is clear.  This function does not
 * initialize the CR bit alias if it has not already been loaded.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: CR bit index.
 * [Return value]
 *     RTL register index.
 */
static inline int test_crb(GuestPPCContext * const ctx, int index)
{
    if (ctx->live.crb[index]) {
        return ctx->live.crb[index];
    } else {
        RTLUnit * const unit = ctx->unit;
        int reg;
        ASSERT(ctx->alias.crb[index]);
        if (ctx->crb_loaded & (0x80000000u >> index)) {
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.crb[index]);
            ctx->live.crb[index] = reg;
        } else {
            const int cr = get_cr(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         reg, cr, 0, (int32_t)(0x80000000u >> index));
        }
        return reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_gpr, set_fpr, set_cr, set_crb, set_lr, set_ctr, set_xer, set_fpscr:
 * Store the given RTL register to the given PowerPC register.  These
 * functions do not add a SET_ALIAS instruction.
 *
 * Complex instructions whose translation includes conditionally-executed
 * code paths must not call these functions from any code which is
 * conditionally executed.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (set_gpr(), set_fpr()) or CR bit index
 *         (set_crb()).
 *     reg: Register to store.
 */
static inline void set_gpr(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.gpr[index] = reg;
    ctx->gpr_dirty |= 1u << index;
}

static inline void set_fpr(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.fpr[index] = reg;
    ctx->fpr_dirty |= 1u << index;
}

static inline void set_cr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.cr = reg;
    ctx->cr_dirty = 1;
}

static inline void set_crb(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.crb[index] = reg;
    ctx->crb_dirty |= 1u << index;
    ctx->crb_loaded |= 0x80000000u >> index;
}

static inline void set_lr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.lr = reg;
    ctx->lr_dirty = 1;
}

static inline void set_ctr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.ctr = reg;
    ctx->ctr_dirty = 1;
}

static inline void set_xer(GuestPPCContext * const ctx, int reg)
{
    ctx->live.xer = reg;
    ctx->xer_dirty = 1;
}

static inline void set_fpscr(GuestPPCContext * const ctx, int reg)
{
    ctx->live.fpscr = reg;
    ctx->fpscr_dirty = 1;
}

/*-----------------------------------------------------------------------*/

/**
 * get_ea_base:  Allocate and return a new RTL register of ADDRESS type
 * containing the host address for the base EA (without offset) in the
 * given D-form instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 * [Return value]
 *     RTL register containing the host address corresponding to (rA|0).
 */
static inline int get_ea_base(GuestPPCContext *ctx, uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (insn_rA(insn)) {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, address, rA, 0, 0);
        const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     host_address, ctx->membase_reg, address, 0);
        return host_address;
    } else {
        return ctx->membase_reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_ea_indexed:  Allocate and return a new RTL register of ADDRESS type
 * containing the host address for the EA in the given X-form instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     guest_ea_ret: Pointer to variable to receive the RTL register
 *         containing ((rA|0) + rB), or NULL if not needed.
 * [Return value]
 *     RTL register containing the host address corresponding to ((rA|0) + rB).
 */
static inline int get_ea_indexed(GuestPPCContext *ctx, uint32_t insn,
                                 int *guest_ea_ret)
{
    RTLUnit * const unit = ctx->unit;

    int addr32;
    if (insn_rA(insn)) {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        addr32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, addr32, rA, rB, 0);
    } else {
        addr32 = get_gpr(ctx, insn_rB(insn));
    }
    if (guest_ea_ret) {
        *guest_ea_ret = addr32;
    }
    const int address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ZCAST, address, addr32, 0, 0);
    const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ADD, host_address, ctx->membase_reg, address, 0);
    return host_address;
}

/*-----------------------------------------------------------------------*/

/**
 * store_live_regs:  Copy live shadow registers to their respective aliases.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     clean: True to clear all dirty flags after storing dirty registers;
 *         false to leave dirty flags alone.  If false, CR bit registers
 *         will not be stored (but they will be merged to the full CR word,
 *         which will be stored).
 *     clear: True to clear all live registers after storing them; false
 *         to leave them live.
 */
static void store_live_regs(GuestPPCContext *ctx, bool clean, bool clear)
{
    RTLUnit * const unit = ctx->unit;

    uint32_t gpr_dirty = ctx->gpr_dirty;
    while (gpr_dirty) {
        const int index = ctz32(gpr_dirty);
        gpr_dirty ^= 1u << index;
        ASSERT(ctx->live.gpr[index]);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.gpr[index], 0, ctx->alias.gpr[index]);
    }

    uint32_t fpr_dirty = ctx->fpr_dirty;
    while (fpr_dirty) {
        const int index = ctz32(fpr_dirty);
        fpr_dirty ^= 1u << index;
        ASSERT(ctx->live.fpr[index]);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.fpr[index], 0, ctx->alias.fpr[index]);
    }

    if (clean) {
        uint32_t crb_dirty = ctx->crb_dirty;
        while (crb_dirty) {
            const int index = ctz32(crb_dirty);
            crb_dirty ^= 1u << index;
            ASSERT(ctx->live.crb[index]);
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.crb[index], 0, ctx->alias.crb[index]);
        }
    }

    if (ctx->cr_dirty) {
        ASSERT(ctx->live.cr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.cr, 0, ctx->alias.cr);
    }

    if (ctx->lr_dirty) {
        ASSERT(ctx->live.lr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.lr, 0, ctx->alias.lr);
    }

    if (ctx->ctr_dirty) {
        ASSERT(ctx->live.ctr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.ctr, 0, ctx->alias.ctr);
    }

    if (ctx->xer_dirty) {
        ASSERT(ctx->live.xer);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.xer, 0, ctx->alias.xer);
    }

    if (ctx->fpscr_dirty) {
        ASSERT(ctx->live.fpscr);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, ctx->live.fpscr, 0, ctx->alias.fpscr);
    }

    if (clean) {
        ctx->gpr_dirty = 0;
        ctx->fpr_dirty = 0;
        ctx->crb_dirty = 0;
        ctx->cr_dirty = 0;
        ctx->lr_dirty = 0;
        ctx->ctr_dirty = 0;
        ctx->xer_dirty = 0;
        ctx->fpscr_dirty = 0;
    }
    if (clear) {
        memset(&ctx->live, 0, sizeof(ctx->live));
    }
}

/*-----------------------------------------------------------------------*/

/**
 * merge_cr:  Merge all CR bit aliases (and untouched CR bits from the
 * processor state block) into a 32-bit CR word.  Helper for
 * guest_ppc_flush_cr().
 *
 * [Parameters]
 *     ctx: Translation context.
 *     make_live: True to always leave CR live in its alias, false to not
 *         call get_cr() if CR is not live.
 * [Return value]
 *     RTL register containing merged value of CR.
 */
static int merge_cr(GuestPPCContext *ctx, bool make_live)
{
    RTLUnit * const unit = ctx->unit;

    uint32_t crb_dirty = ctx->crb_loaded & ctx->crb_changed;
    ASSERT(crb_dirty != 0);  // We won't be called if there's nothing to merge.

    int cr;
    if (crb_dirty == ~UINT32_C(0)) {
        cr = 0;
    } else {
        int old_cr;
        if (make_live) {
            old_cr = get_cr(ctx);
        } else if (ctx->live.cr) {
            old_cr = ctx->live.cr;
        } else {
            old_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS, old_cr, 0, 0, ctx->alias.cr);
        }
        cr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, cr, old_cr, 0, (int32_t)~crb_dirty);
    }

    while (crb_dirty) {
        const int bit = clz32(crb_dirty);
        crb_dirty ^= 0x80000000u >> bit;
        const int crbN = get_crb(ctx, bit);
        int shifted_crbN;
        if (bit == 31) {
            shifted_crbN = crbN;
        } else {
            shifted_crbN = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, shifted_crbN, crbN, 0, 31 - bit);
        }
        if (cr) {
            const int new_cr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, new_cr, cr, shifted_crbN, 0);
            cr = new_cr;
        } else {
            cr = shifted_crbN;
        }
    }

    return cr;
}

/*-----------------------------------------------------------------------*/

/**
 * post_insn_callback:  Add RTL to call the post-instruction callback if
 * one has been set.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Instruction address to pass to the callback.
 */
static void post_insn_callback(GuestPPCContext *ctx, uint32_t address)
{
    if (ctx->handle->post_insn_callback) {
        store_live_regs(ctx, false, false);
        guest_ppc_flush_cr(ctx, false);
        RTLUnit * const unit = ctx->unit;
        const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, func, 0, 0,
                     (uintptr_t)ctx->handle->post_insn_callback);
        rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT,
                     0, func, ctx->psb_reg, rtl_imm32(unit, address));
    }
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
    set_nia(ctx, rtl_imm32(ctx->unit, nia));
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
    const int xer = get_xer(ctx);
    const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, so, xer, 0, XER_SO_SHIFT | 1<<8);
    set_crb(ctx, 0, lt);
    set_crb(ctx, 1, gt);
    set_crb(ctx, 2, eq);
    set_crb(ctx, 3, so);
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

    const int rA = get_gpr(ctx, insn_rA(insn));
    const int32_t imm = shift_imm ? insn_SIMM(insn)<<16 : insn_SIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rA, 0, imm);
    set_gpr(ctx, insn_rD(insn), result);

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

    int srcA = get_gpr(ctx, insn_rA(insn));
    if (invert_rA) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NOT, inverted, srcA, 0, 0);
        srcA = inverted;
    }

    int srcB = 0;
    if (srcB_sel > 0) {
        srcB = get_gpr(ctx, insn_rB(insn));
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

    set_gpr(ctx, insn_rD(insn), result);

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

    if (insn_OE(insn)) {
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
        const int SO_OV = rtl_imm32(unit, XER_SO | XER_OV);
        const int bits_to_set = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SELECT, bits_to_set, SO_OV, ov, ov);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_OR, new_xer, masked_xer, bits_to_set, 0);
        set_xer(ctx, new_xer);
    }

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_bitmisc:  Translate the miscellaneous bit manipulation
 * instructions (cntlzw, extsh, extsb).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL instruction to perform the operation.
 */
static inline void translate_bitmisc(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, insn_rS(insn));
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, 0, 0);
    set_gpr(ctx, insn_rA(insn), result);

    if (insn_Rc(insn)) {
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
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     target: Branch target address.
 *     target_label: RTL label for the target block.
 */
static void translate_branch_label(
    GuestPPCContext *ctx, uint32_t address, int BO, int BI,
    uint32_t target, int target_label)
{
    const binrec_t * const handle = ctx->handle;
    RTLUnit * const unit = ctx->unit;

    int skip_label;
    if ((BO & 0x14) == 0
     || ((BO & 0x14) != 0x14 && handle->use_branch_callback)) {
        /* Need an extra label in case a non-final test fails. */
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

        /* Store here so any update to CTR is stored along with other pending
         * changes. */
        store_live_regs(ctx, true, false);

        if (skip_label) {
            rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                         0, new_ctr, 0, skip_label);
        } else {
            branch_op = BO & 0x02 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ;
            test_reg = new_ctr;
        }
    } else {
        store_live_regs(ctx, true, false);
    }
    /* All dirty registers have been stored at this point. */

    if (!(BO & 0x10)) {
        const int test = test_crb(ctx, BI);
        if (handle->use_branch_callback) {
            rtl_add_insn(unit, BO & 0x08 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ,
                         0, test, 0, skip_label);
        } else {
            branch_op = BO & 0x08 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z;
            test_reg = test;
        }
    }

    if (handle->use_branch_callback || handle->post_insn_callback) {
        set_nia_imm(ctx, target);
    }
    post_insn_callback(ctx, address);
    if (handle->use_branch_callback) {
        ASSERT(branch_op == RTLOP_GOTO);
        const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD, func, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_branch_callback);
        const int rtl_address = rtl_imm32(unit, address);
        const int callback_result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_CALL,
                     callback_result, func, ctx->psb_reg, rtl_address);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                     0, callback_result, 0, ctx->epilogue_label);
    }

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
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     LK: LK bit of instruction word.
 *     target: RTL register containing the branch target address.
 *     clear_low_bits: True to clear the low 2 bits of the target address
 *         (as for BCLR/BCCTR), false if the low bits are known to be 0b00.
 */
static void translate_branch_terminal(
    GuestPPCContext *ctx, uint32_t address, int BO, int BI, int LK,
    int target, bool clear_low_bits)
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

        store_live_regs(ctx, true, false);

        rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                     0, new_ctr, 0, skip_label);
    } else {
        store_live_regs(ctx, true, false);
    }

    if (!(BO & 0x10)) {
        const int test = test_crb(ctx, BI);
        rtl_add_insn(unit, BO & 0x08 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ,
                     0, test, 0, skip_label);
    }

    if (LK) {
        /* Write LR directly rather than going through set_lr() so we don't
         * leak the modified LR to the not-taken code path. */
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, rtl_imm32(unit, address+4), 0, ctx->alias.lr);
    }
    set_nia(ctx, target);
    post_insn_callback(ctx, address);
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
 *     address: Address of instruction being translated.
 *     BO: BO field of instruction word (0x14 for an unconditional branch).
 *     BI: BI field of instruction word (ignored if an unconditional branch).
 *     disp: Displacement field of instruction word.
 *     AA: AA bit of instruction word.
 *     LK: LK bit of instruction word.
 */
static inline void translate_branch(
    GuestPPCContext *ctx, uint32_t address, int BO, int BI, int32_t disp,
    int AA, int LK)
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
        translate_branch_label(ctx, address, BO, BI, target, target_label);
    } else {
        translate_branch_terminal(ctx, address, BO, BI, LK,
                                  rtl_imm32(unit, target), false);
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

    const int rA = get_gpr(ctx, insn_rA(insn));

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    if (is_imm) {
        const int32_t imm =
            is_signed ? insn_SIMM(insn) : (int32_t)insn_UIMM(insn);
        rtl_add_insn(unit, is_signed ? RTLOP_SLTSI : RTLOP_SLTUI,
                     lt, rA, 0, imm);
        rtl_add_insn(unit, is_signed ? RTLOP_SGTSI : RTLOP_SGTUI,
                     gt, rA, 0, imm);
        rtl_add_insn(unit, RTLOP_SEQI, eq, rA, 0, imm);
    } else {
        const int32_t rB = get_gpr(ctx, insn_rB(insn));
        rtl_add_insn(unit, is_signed ? RTLOP_SLTS : RTLOP_SLTU, lt, rA, rB, 0);
        rtl_add_insn(unit, is_signed ? RTLOP_SGTS : RTLOP_SGTU, gt, rA, rB, 0);
        rtl_add_insn(unit, RTLOP_SEQ, eq, rA, rB, 0);
    }

    const int xer = get_xer(ctx);
    const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, so, xer, 0, XER_SO_SHIFT | 1<<8);

    set_crb(ctx, insn_crfD(insn)*4+0, lt);
    set_crb(ctx, insn_crfD(insn)*4+1, gt);
    set_crb(ctx, insn_crfD(insn)*4+2, eq);
    set_crb(ctx, insn_crfD(insn)*4+3, so);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_gpr:  Translate an integer load or store instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL instruction to perform the operation (assuming a
 *         same-endian host).
 *     is_store: True if the instruction is a store instruction.
 *     is_indexed: True if the access is an indexed access (like lwx or stwx).
 *     update: True if register rA should be updated with the final EA.
 */
static inline void translate_load_store_gpr(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool is_store,
    bool is_indexed, bool update)
{
    RTLUnit * const unit = ctx->unit;

    if (ctx->handle->host_little_endian) {
        ASSERT(rtlop != RTLOP_LOAD_S16_BR);  // No such PowerPC instruction.
        switch (rtlop) {
            case RTLOP_LOAD:         rtlop = RTLOP_LOAD_BR;      break;
            case RTLOP_LOAD_U16:     rtlop = RTLOP_LOAD_U16_BR;  break;
            case RTLOP_LOAD_S16:     rtlop = RTLOP_LOAD_S16_BR;  break;
            case RTLOP_STORE:        rtlop = RTLOP_STORE_BR;     break;
            case RTLOP_STORE_I16:    rtlop = RTLOP_STORE_I16_BR; break;
            case RTLOP_LOAD_BR:      rtlop = RTLOP_LOAD;         break;
            case RTLOP_LOAD_U16_BR:  rtlop = RTLOP_LOAD_U16;     break;
            case RTLOP_STORE_BR:     rtlop = RTLOP_STORE;        break;
            case RTLOP_STORE_I16_BR: rtlop = RTLOP_STORE_I16;    break;
            default: break;
        }
    }

    int ea, host_address, disp;
    if (update) {
        ASSERT(insn_rA(insn) != 0);
        if (is_indexed) {
            host_address = get_ea_indexed(ctx, insn, &ea);
        } else {
            const int rA = get_gpr(ctx, insn_rA(insn));
            if (insn_d(insn) != 0) {
                ea = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ADDI, ea, rA, 0, insn_d(insn));
            } else {
                ea = rA;
            }
            const int ea_zcast = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ZCAST, ea_zcast, ea, 0, 0);
            host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ADD,
                         host_address, ctx->membase_reg, ea_zcast, 0);
        }
        disp = 0;
    } else {
        ea = 0;  // Not used, but required to avoid an "uninitialized" warning.
        if (is_indexed) {
            host_address = get_ea_indexed(ctx, insn, NULL);
            disp = 0;
        } else {
            if (insn_rA(insn) != 0 || insn_d(insn) >= 0) {
                host_address = get_ea_base(ctx, insn);
                disp = insn_d(insn);
            } else {
                const int offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
                rtl_add_insn(unit, RTLOP_LOAD_IMM,
                             offset, 0, 0, (uint32_t)insn_d(insn));
                host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
                rtl_add_insn(unit, RTLOP_ADD,
                             host_address, ctx->membase_reg, offset, 0);
                disp = 0;
            }
        }
    }

    if (is_store) {
        const int value = get_gpr(ctx, insn_rS(insn));
        rtl_add_insn(unit, rtlop, 0, host_address, value, disp);
    } else {
        const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, rtlop, value, host_address, 0, disp);
        set_gpr(ctx, insn_rD(insn), value);
    }

    if (update) {
        if (is_indexed || insn_d(insn) != 0) {
            set_gpr(ctx, insn_rA(insn), ea);
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_multiple:  Translate a load or store multiple
 * instruction (lmw/stmw).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_store: True if the instruction is a store instruction (stmw).
 */
static inline void translate_load_store_multiple(
    GuestPPCContext *ctx, uint32_t insn, bool is_store)
{
    RTLUnit * const unit = ctx->unit;

    RTLOpcode rtlop = is_store ? RTLOP_STORE : RTLOP_LOAD;
    if (ctx->handle->host_little_endian) {
        rtlop = (is_store ? RTLOP_STORE_BR : RTLOP_LOAD_BR);
    }

    int host_address = get_ea_base(ctx, insn);
    int disp = insn_d(insn);
    if (disp >= (int)(32768 - 4*(31-insn_rD(insn)))) {
        /* Advancing the offset will wrap around to negative values!
         * Add it into the base address as a workaround. */
        const int base = host_address;
        host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADDI, host_address, base, 0, disp);
        disp = 0;
    }

    int rD = insn_rD(insn);
    int reg[4];

    /* Copy data in batches of 4 registers to minimize load stalls. */

    if (is_store) {
        for (int i = rD; i & 3; i++) {
            reg[i & 3] = get_gpr(ctx, i);
        }
        for (; rD & 3; rD++, disp += 4) {
            rtl_add_insn(unit, rtlop, 0, host_address, reg[rD & 3], disp);
        }
    } else {
        for (int i = rD; i & 3; i++, disp += 4) {
            reg[i & 3] = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, rtlop, reg[i & 3], host_address, 0, disp);
        }
        for (; rD & 3; rD++) {
            set_gpr(ctx, rD, reg[rD & 3]);
        }
    }

    for (; rD < 32; rD += 4) {
        if (is_store) {
            for (int i = 0; i < 4; i++) {
                reg[i] = get_gpr(ctx, rD+i);
            }
            for (int i = 0; i < 4; i++, disp += 4) {
                rtl_add_insn(unit, rtlop, 0, host_address, reg[i], disp);
            }
        } else {
            for (int i = 0; i < 4; i++, disp += 4) {
                reg[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, rtlop, reg[i], host_address, 0, disp);
            }
            for (int i = 0; i < 4; i++) {
                set_gpr(ctx, rD+i, reg[i]);
            }
        }
    }

    /* Flush loaded GPRs so we don't have a bunch of dirty RTL registers
     * live until the end of the block. */
    if (!is_store) {
        store_live_regs(ctx, true, false);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_string:  Translate a string load or store
 * instruction (lswi/lswx/stswi/stswx).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_store: True if the instruction is a store instruction (stswi/stswx).
 *     is_imm: True if the instruction has an immediate count (lswi/stswi).
 */
static inline void translate_load_store_string(
    GuestPPCContext *ctx, uint32_t insn, bool is_store, bool is_imm)
{
    RTLUnit * const unit = ctx->unit;

    /* We implement the string move instructions by loading or storing
     * directly to/from the PSB, so make sure it's up to date, and clear
     * the live register set so no future code tries to use stale values. */
    store_live_regs(ctx, true, true);

    int base_address, host_address;
    if (insn_rA(insn)) {
        const int rA = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     rA, 0, 0, ctx->alias.gpr[insn_rA(insn)]);
        const int rA_zcast = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, rA_zcast, rA, 0, 0);
        base_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     base_address, ctx->membase_reg, rA_zcast, 0);
    } else {
        base_address = ctx->membase_reg;
    }
    if (is_imm) {
        host_address = base_address;
    } else {
        const int rB = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     rB, 0, 0, ctx->alias.gpr[insn_rB(insn)]);
        const int rB_zcast = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, rB_zcast, rB, 0, 0);
        host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     host_address, base_address, rB_zcast, 0);
    }

    const int psb_reg = ctx->psb_reg;
    const int gpr_base = ctx->handle->setup.state_offset_gpr;
    const int endian_flip = ctx->handle->host_little_endian ? 3 : 0;

    if (is_imm) {

        const int n = insn_NB(insn) ? insn_NB(insn) : 32;

        /* Unroll into units of 4 bytes, both to try and hide load latency
         * and since each GPR is 4 bytes wide anyway. */
        int rD = insn_rD(insn);
        for (int i = 0; i+4 <= n; i += 4, rD = (rD + 1) & 31) {
            int byte_reg[4];
            for (int byte = 0; byte < 4; byte++) {
                byte_reg[byte] = rtl_alloc_register(unit, RTLTYPE_INT32);
            }
            const int gpr_offset = gpr_base + 4*rD;
            if (is_store) {
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], psb_reg, 0,
                                 gpr_offset + (byte ^ endian_flip));
                }
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, host_address, byte_reg[byte], i + byte);
                }
            } else {
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], host_address, 0, i + byte);
                }
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, psb_reg, byte_reg[byte],
                                 gpr_offset + (byte ^ endian_flip));
                }
            }
        }

        if ((n & 3) != 0) {
            const int i = n & ~3;
            int byte_reg[4];
            for (int byte = 0; byte < (n & 3); byte++) {
                byte_reg[byte] = rtl_alloc_register(unit, RTLTYPE_INT32);
            }
            const int gpr_offset = gpr_base + 4*rD;
            if (is_store) {
                for (int byte = 0; byte < (n & 3); byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], psb_reg, 0,
                                 gpr_offset + (byte ^ endian_flip));
                }
                for (int byte = 0; byte < (n & 3); byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, host_address, byte_reg[byte], i + byte);
                }
            } else {
                for (int byte = 0; byte < (n & 3); byte++) {
                    rtl_add_insn(unit, RTLOP_LOAD_U8,
                                 byte_reg[byte], host_address, 0, i + byte);
                }
                const int zero = rtl_imm32(unit, 0);
                for (int byte = (n & 3); byte < 4; byte++) {
                    byte_reg[byte] = zero;
                }
                for (int byte = 0; byte < 4; byte++) {
                    rtl_add_insn(unit, RTLOP_STORE_I8,
                                 0, psb_reg, byte_reg[byte],
                                 gpr_offset + (byte ^ endian_flip));
                }
            }
        }

    } else {  // !is_imm

        /* We don't even attempt to optimize this because it's way too
         * complicated already.  Hopefully nobody actually uses lswx/stswx
         * anymore. */

        const int xer = get_xer(ctx);
        const int xer_count = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, xer_count, xer, 0, 127);

        /* We need to zero out the unused bytes of the last register if
         * not loading a multiple of four bytes.  Do this by clearing
         * the entire register ahead of time for simplicity. */
        if (!is_store) {
            const int count_mod_4 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, count_mod_4, xer_count, 0, 3);
            const int start_label = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, count_mod_4, 0, start_label);

            const int last_gpr_temp = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ADDI, last_gpr_temp, xer_count, 0,
                         4 * insn_rD(insn));
            const int last_gpr_offset =
                rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         last_gpr_offset, last_gpr_temp, 0, 31<<2);
            const int last_gpr_offset_zcast =
                rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ZCAST,
                         last_gpr_offset_zcast, last_gpr_offset, 0, 0);
            const int last_gpr_address =
                rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_ADD,
                         last_gpr_address, psb_reg, last_gpr_offset_zcast, 0);
            rtl_add_insn(unit, RTLOP_STORE,
                         0, last_gpr_address, rtl_imm32(unit, 0), gpr_base);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, start_label);
        }

        /* We use ADDRESS type for the count and GPR offset aliases so we
         * can add them directly to the base addresses without an
         * intermediate ZCAST. */
        const int count = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, count, xer_count, 0, 0);
        const int init_i = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD_IMM, init_i, 0, 0, 0);
        const int alias_i = rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, init_i, 0, alias_i);
        const int init_gpr_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     init_gpr_offset, 0, 0, 4 * insn_rD(insn));
        const int alias_gpr_offset =
            rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, init_gpr_offset, 0, alias_gpr_offset);

        const int loop_label = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, loop_label);
        const int i = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, i, 0, 0, alias_i);
        const int i_test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTU, i_test, i, count, 0);
        const int end_label = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, i_test, 0, end_label);
        const int gpr_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     gpr_offset, 0, 0, alias_gpr_offset);

        const int mem_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD, mem_address, host_address, i, 0);

        int real_offset = gpr_offset;
        if (endian_flip) {
            real_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_XORI,
                         real_offset, gpr_offset, 0, endian_flip);
        }
        int gpr_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD, gpr_address, psb_reg, real_offset, 0);

        const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
        if (is_store) {
            rtl_add_insn(unit, RTLOP_LOAD_U8, value, gpr_address, 0, gpr_base);
            rtl_add_insn(unit, RTLOP_STORE_I8, 0, mem_address, value, 0);
        } else {
            rtl_add_insn(unit, RTLOP_LOAD_U8, value, mem_address, 0, 0);
            rtl_add_insn(unit, RTLOP_STORE_I8, 0, gpr_address, value, gpr_base);
        }

        int new_i = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADDI, new_i, i, 0, 1);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_i, 0, alias_i);
        const int gpr_offset_temp = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADDI, gpr_offset_temp, gpr_offset, 0, 1);
        int new_gpr_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ANDI,
                     new_gpr_offset, gpr_offset_temp, 0, 127);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, new_gpr_offset, 0, alias_gpr_offset);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, loop_label);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, end_label);

    }  // if (is_imm)
}

/*-----------------------------------------------------------------------*/

/**
 * translate_logic_crb:  Translate a logic instruction operating on CR bits.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-register instruction to perform the operation.
 *     invert_crbB: True if the value of crbB should be inverted.
 *     invert_result: True if the result should be inverted.
 */
static inline void translate_logic_crb(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool invert_crbB,
    bool invert_result)
{
    RTLUnit * const unit = ctx->unit;

    const int crbA = get_crb(ctx, insn_crbA(insn));

    int crbB = get_crb(ctx, insn_crbB(insn));
    if (invert_crbB) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, inverted, crbB, 0, 1);
        crbB = inverted;
    }

    int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, crbA, crbB, 0);
    if (invert_result) {
        const int inverted = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_XORI, inverted, result, 0, 1);
        result = inverted;
    }
    set_crb(ctx, insn_crbD(insn), result);
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

    const int rS = get_gpr(ctx, insn_rS(insn));
    const uint32_t imm = shift_imm ? insn_UIMM(insn)<<16 : insn_UIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, 0, (int32_t)imm);
    set_gpr(ctx, insn_rA(insn), result);

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
 *     rtlop: RTL register-register instruction to perform the operation.
 *     invert_rB: True if the value of rB should be inverted.
 *     invert_result: True if the result should be inverted.
 */
static inline void translate_logic_reg(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool invert_rB,
    bool invert_result)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, insn_rS(insn));

    int rB = get_gpr(ctx, insn_rB(insn));
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
    set_gpr(ctx, insn_rA(insn), result);

    if (insn_Rc(insn)) {
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

    const int spr = insn_spr(insn);

    switch (spr) {
      case SPR_XER:
        if (to_spr) {
            set_xer(ctx, get_gpr(ctx, insn_rS(insn)));
        } else {
            set_gpr(ctx, insn_rD(insn), get_xer(ctx));
        }
        break;

      case SPR_LR:
        if (to_spr) {
            set_lr(ctx, get_gpr(ctx, insn_rS(insn)));
        } else {
            set_gpr(ctx, insn_rD(insn), get_lr(ctx));
        }
        break;

      case SPR_CTR:
        if (to_spr) {
            set_ctr(ctx, get_gpr(ctx, insn_rS(insn)));
        } else {
            set_gpr(ctx, insn_rD(insn), get_ctr(ctx));
        }
        break;

      case SPR_TBL:
      case SPR_TBU:
        if (to_spr) {
            log_warning(ctx->handle, "0x%X: Invalid attempt to write TB%c",
                        address, spr==SPR_TBU ? 'U' : 'L');
            rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        } else {
            /* We have to manually store rD since we set it differently
             * depending on whether a timebase handler function is present. */
            const int rD = insn_rD(insn);
            if (ctx->gpr_dirty & (1u << rD)) {
                ctx->gpr_dirty ^= 1u << rD;
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, ctx->live.gpr[rD], 0, ctx->alias.gpr[rD]);
            }
            ctx->live.gpr[rD] = 0;

            const int label_no_handler = rtl_alloc_label(unit);
            const int label_end = rtl_alloc_label(unit);
            const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_LOAD, func, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_timebase_handler);
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, func, 0, label_no_handler);

            const int result64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_CALL, result64, func, ctx->psb_reg, 0);
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            if (spr == SPR_TBL) {
                rtl_add_insn(unit, RTLOP_ZCAST, result, result64, 0, 0);
            } else {
                const int shifted = rtl_alloc_register(unit, RTLTYPE_INT64);
                rtl_add_insn(unit, RTLOP_SRLI, shifted, result64, 0, 32);
                rtl_add_insn(unit, RTLOP_ZCAST, result, shifted, 0, 0);
            }
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, result, 0, ctx->alias.gpr[rD]);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_end);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_no_handler);
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, rtl_imm32(unit,0), 0, ctx->alias.gpr[rD]);

            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_end);
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
            const int rS = get_gpr(ctx, insn_rS(insn));
            rtl_add_insn(unit, RTLOP_STORE, 0, ctx->psb_reg, rS,
                         ctx->handle->setup.state_offset_gqr + 4 * (spr & 7));
            // FIXME: this should stop GQR optimization when that's implemented
        } else {
            const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_LOAD, value, ctx->psb_reg, 0,
                         ctx->handle->setup.state_offset_gqr + 4 * (spr & 7));
            set_gpr(ctx, insn_rD(insn), value);
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

    const int rA = get_gpr(ctx, insn_rA(insn));
    const int rB = get_gpr(ctx, insn_rB(insn));

    /* For division, we might skip over the actual division operation, so
     * store the target register now.  We handle XER (when OE is set)
     * separately, since we have to set SO|OV anyway on the overflow path. */
    const bool is_divide = (rtlop == RTLOP_DIVU || rtlop == RTLOP_DIVS);
    if (is_divide) {
        const int rD = insn_rD(insn);
        if (ctx->gpr_dirty & (1u << rD)) {
            ctx->gpr_dirty ^= 1u << rD;
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.gpr[rD], 0, ctx->alias.gpr[rD]);
        }
        ctx->live.gpr[rD] = 0;
    }

    int div_skip_label = 0;
    int xer = 0;
    if (is_divide) {
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
    if (is_divide) {
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, result, 0, ctx->alias.gpr[insn_rD(insn)]);
    } else {
        set_gpr(ctx, insn_rD(insn), result);
    }

    if (do_overflow) {
        if (!xer) {
            xer = get_xer(ctx);
        }
        const int masked_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, (int32_t)~XER_OV);
        if (rtlop == RTLOP_MUL) {
            /* mullwo's overflow check is for signed integers, so we can't
             * just check for the high word being nonzero. */
            const int result_hi = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_MULHS, result_hi, rA, rB, 0);
            const int lo_sign = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SRAI, lo_sign, result, 0, 31);
            const int overflow = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_XOR, overflow, result_hi, lo_sign, 0);
            const int SO_OV = rtl_imm32(unit, XER_SO | XER_OV);
            const int bits_to_set = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SELECT,
                         bits_to_set, SO_OV, overflow, overflow);
            const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, new_xer, masked_xer, bits_to_set, 0);
            set_xer(ctx, new_xer);
        } else {
            ASSERT(rtlop == RTLOP_DIVU || rtlop == RTLOP_DIVS);
            set_xer(ctx, masked_xer);
        }
    }

    if (div_skip_label) {
        int div_continue_label = 0;
        if (do_overflow) {
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, ctx->live.xer, 0, ctx->alias.xer);
            ctx->live.xer = 0;
            ctx->xer_dirty = 0;
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

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_rotate_mask:  Translate a rlwinm, rlwnm, or rlwimi instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_imm: True if the shift count is an immediate value, false if rB.
 *     insert: True for a rlwimi instruction.
 */
static void translate_rotate_mask(
    GuestPPCContext *ctx, uint32_t insn, bool is_imm, bool insert)
{
    RTLUnit * const unit = ctx->unit;

    const int SH = insn_SH(insn);
    const int MB = insn_MB(insn);
    const int ME = insn_ME(insn);

    const int rS = get_gpr(ctx, insn_rS(insn));
    int result;

    if (MB == ((ME + 1) & 31)) {  // rotlw/rotlwi
        if (is_imm) {
            if (SH == 0) {
                result = rS;
            } else {
                result = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI, result, rS, 0, 32-SH);
            }
        } else {
            const int rB = get_gpr(ctx, insn_rB(insn));
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ROL, result, rS, rB, 0);
        }

    } else if (is_imm && !insert && MB == 0 && ME == 31-SH) {  // slwi
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLLI, result, rS, 0, SH);

    } else if (is_imm && !insert && MB == 32-SH && ME == 31) {  // srwi
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SRLI, result, rS, 0, 32-SH);

    } else if (insert) {
        ASSERT(is_imm);
        const int rA = get_gpr(ctx, insn_rA(insn));
        if (MB <= ME) {
            const int start = 31 - ME;
            const int count = ME - MB + 1;
            const int base = rA;
            int value;
            if (SH == start) {
                value = rS;
            } else {
                value = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI,
                             value, rS, 0, ((32-SH) + start) & 31);
            }
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFINS,
                         result, base, value, start | count<<8);
        } else {
            const int start = 32 - MB;
            const int count = MB - ME - 1;
            ASSERT(count > 0);  // Or else it would be rotlwi.
            int rS_rotated;
            if (SH == 0) {
                rS_rotated = rS;
            } else {
                rS_rotated = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI, rS_rotated, rS, 0, 32-SH);
            }
            const uint32_t mask = ((1u << count) - 1) << start;
            const int rS_masked = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI,
                         rS_masked, rS_rotated, 0, (int32_t)~mask);
            const int rA_masked = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, rA_masked, rA, 0, (int32_t)mask);
            result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, result, rS_masked, rA_masked, 0);
        }

    } else {
        /* MASK function for little-endian bit numbering.  Assumes mb >= me. */
        #define MASK_LE(mb, me) \
            ((uint32_t)((UINT64_C(1) << ((mb)-(me)+1)) - 1) << (me))
        const int mb = 31 - MB;
        const int me = 31 - ME;
        const uint32_t mask =
            (mb < me ? ~MASK_LE((me-1) & 31, (mb+1) & 31) : MASK_LE(mb, me));
        #undef MASK_LE

        int rotated;
        if (is_imm) {
            if (SH == 0) {
                rotated = rS;
            } else {
                rotated = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_RORI, rotated, rS, 0, 32-SH);
            }
        } else {
            const int rB = get_gpr(ctx, insn_rB(insn));
            rotated = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ROL, rotated, rS, rB, 0);
        }
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, result, rotated, 0, (int32_t)mask);
    }

    set_gpr(ctx, insn_rA(insn), result);

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_shift:  Translate a shift instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     rtlop: RTL register-immediate instruction to perform the operation.
 *     is_imm: True if the shift count is an immediate value, false if rB.
 *     is_sra: True if the shift is an arithmetic right shift (sets XER[CA]).
 */
static void translate_shift(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool is_imm,
    bool is_sra)
{
    RTLUnit * const unit = ctx->unit;

    int rS = get_gpr(ctx, insn_rS(insn));
    int count, result;
    if (is_imm) {
        count = 0;  // Not used, but avoid a "may be uninitialized" warning.
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, rtlop, result, rS, 0, insn_SH(insn));
    } else {
        const int rB = get_gpr(ctx, insn_rB(insn));
        count = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, count, rB, 0, 63);
        const int rS_64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        if (is_sra) {
            rtl_add_insn(unit, RTLOP_SCAST, rS_64, rS, 0, 0);
        } else {
            rtl_add_insn(unit, RTLOP_ZCAST, rS_64, rS, 0, 0);
        }
        rS = rS_64;
        const int result_64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, rtlop, result_64, rS, count, 0);
        result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, result, result_64, 0, 0);
    }
    set_gpr(ctx, insn_rA(insn), result);

    if (is_sra) {
        int test;
        if (is_imm) {
            const int32_t mask = (int32_t)((1u << insn_SH(insn)) - 1);
            test = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, test, rS, 0, mask);
        } else {
            const int one = rtl_imm64(unit, 1);
            const int shifted_one = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_SLL, shifted_one, one, count, 0);
            const int mask = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_ADDI, mask, shifted_one, 0, -1);
            test = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_AND, test, rS, mask, 0);
        }
        const int has_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTUI, has_bits, test, 0, 0);
        const int is_neg = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTSI, is_neg, rS, 0, 0);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_AND, ca, has_bits, is_neg, 0);
        const int xer = get_xer(ctx);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, XER_CA_SHIFT | 1<<8);
        set_xer(ctx, new_xer);
    }

    if (insn_Rc(insn)) {
        update_cr0(ctx, result);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_trap:  Translate a TW or TWI instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 *     rB: RTL register containing second comparison value (rB or immediate).
 */
static void translate_trap(
    GuestPPCContext *ctx, uint32_t address, uint32_t insn, int rB)
{
    RTLUnit * const unit = ctx->unit;

    const int TO = insn_TO(insn);
    if (!TO) {
        return;  // Effectively a NOP.
    }

    const int label = rtl_alloc_label(unit);
    const int rA = get_gpr(ctx, insn_rA(insn));
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

    /* Don't touch dirty flags because this is only executed on the
     * trapping path.  (We assume trapping is the exceptional case,
     * so we don't store registers unconditionally.) */
    store_live_regs(ctx, false, false);
    guest_ppc_flush_cr(ctx, false);
    set_nia_imm(ctx, address);
    const int trap_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_LOAD, trap_handler, ctx->psb_reg, 0,
                 ctx->handle->setup.state_offset_trap_handler);
    rtl_add_insn(unit, RTLOP_CALL, 0, trap_handler, ctx->psb_reg, 0);
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
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 * [Return value]
 *     True on success, false on error.
 */
static inline void translate_x1F(
    GuestPPCContext * const ctx, const uint32_t address, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    switch ((PPCExtendedOpcode1F)insn_XO_10(insn)) {

      /* XO_5 = 0x00 */
      case XO_CMP:
        translate_compare(ctx, insn, false, true);
        return;
      case XO_CMPL:
        translate_compare(ctx, insn, false, false);
        return;
      case XO_MCRXR: {
        const int xer = get_xer(ctx);
        for (int bit = 0; bit < 4; bit++) {
            const int crb = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT, crb, xer, 0, (31-bit) | (1<<8));
            set_crb(ctx, insn_crfD(insn)*4 + bit, crb);
        }
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, new_xer, xer, 0, 0x0FFFFFFF);
        set_xer(ctx, new_xer);
        return;
      }  // case XO_MCRXR

      /* XO_5 = 0x04 */
      case XO_TW:
        translate_trap(ctx, address, insn, get_gpr(ctx, insn_rB(insn)));
        return;

      /* XO_5 = 0x08 */
      case XO_SUBFC:
      case XO_SUBFCO:
        translate_addsub_reg(ctx, insn, 1, 1, true, true);
        return;
      case XO_SUBF: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, rB, rA, 0);
        set_gpr(ctx, insn_rD(insn), result);
        if (insn_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_SUBF
      case XO_SUBFO:
        translate_addsub_reg(ctx, insn, 1, 1, true, false);
        return;
      case XO_NEG: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_NEG, result, rA, 0, 0);
        set_gpr(ctx, insn_rD(insn), result);
        if (insn_Rc(insn)) {
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
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int rB = get_gpr(ctx, insn_rB(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADD, result, rA, rB, 0);
        set_gpr(ctx, insn_rD(insn), result);
        if (insn_Rc(insn)) {
            update_cr0(ctx, result);
        }
        return;
      }  // case XO_ADD
      case XO_ADDO:
        translate_addsub_reg(ctx, insn, 1, 0, false, false);
        return;

      /* XO_5 = 0x0B */
      case XO_MULHWU:
      case XO_UNDOCUMENTED_MULHWUO:  // OE is ignored.
        translate_muldiv_reg(ctx, insn, RTLOP_MULHU, false);
        return;
      case XO_MULHW:
      case XO_UNDOCUMENTED_MULHWO:  // OE is ignored.
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
      case XO_MTCRF:
        if (insn_CRM(insn) == 0xFF) {
            set_cr(ctx, get_gpr(ctx, insn_rS(insn)));
            memset(ctx->live.crb, 0, sizeof(ctx->live.crb));
            ctx->crb_loaded = 0;
            ctx->crb_dirty = 0;
        } else {
            const int rS = get_gpr(ctx, insn_rS(insn));
            for (int i = 0; i < 8; i++) {
                if (insn_CRM(insn) & (0x80 >> i)) {
                    for (int bit = i*4; bit < i*4+4; bit++) {
                        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_BFEXT,
                                     reg, rS, 0, (31-bit) | (1<<8));
                        set_crb(ctx, bit, reg);
                    }
                }
            }
        }
        return;

      /* XO_5 = 0x12 */
      case XO_MTMSR:
      case XO_MTSR:
      case XO_MTSRIN:
      case XO_TLBIE:
        translate_unimplemented_insn(ctx, address, insn);
        return;

      /* XO_5 = 0x13 */
      case XO_MFCR:
        guest_ppc_flush_cr(ctx, true);
        set_gpr(ctx, insn_rD(insn), get_cr(ctx));
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

      /* XO_5 = 0x14 */
      case XO_LWARX: {
        const binrec_t *handle = ctx->handle;
        const int psb_reg = ctx->psb_reg;
        const RTLOpcode rtlop =
            handle->host_little_endian ? RTLOP_LOAD_BR : RTLOP_LOAD;
        const int host_address = get_ea_indexed(ctx, insn, NULL);
        const int value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, rtlop, value, host_address, 0, 0);
        set_gpr(ctx, insn_rD(insn), value);
        rtl_add_insn(unit, RTLOP_STORE_I8, 0, psb_reg, rtl_imm32(unit, 1),
                     handle->setup.state_offset_reserve_flag);
        rtl_add_insn(unit, RTLOP_STORE, 0, psb_reg, value,
                     handle->setup.state_offset_reserve_state);
        return;
      }  // case XO_LWARX

      /* XO_5 = 0x15 */
      case XO_LSWX:
        translate_load_store_string(ctx, insn, false, false);
        return;
      case XO_LSWI:
        translate_load_store_string(ctx, insn, false, true);
        return;
      case XO_STSWX:
        translate_load_store_string(ctx, insn, true, false);
        return;
      case XO_STSWI:
        translate_load_store_string(ctx, insn, true, true);
        return;

      /* XO_5 = 0x16 */
      case XO_DCBST:
      case XO_DCBF:
      case XO_DCBTST:
      case XO_DCBT:
      case XO_DCBI:
        // FIXME: We currently act as if there is no data cache.
        return;
      case XO_STWCX_: {
        const binrec_t *handle = ctx->handle;
        const int psb_reg = ctx->psb_reg;

        const int skip_label = rtl_alloc_label(unit);
        const int flag = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_U8, flag, psb_reg, 0,
                     handle->setup.state_offset_reserve_flag);
        const int zero = rtl_imm32(unit, 0);
        set_crb(ctx, 0, zero);
        set_crb(ctx, 1, zero);
        /* Can't use set_crb() for the eq bit because of the conditional
         * branches. */
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, zero, 0, ctx->alias.crb[2]);
        const int xer = get_xer(ctx);
        const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFEXT, so, xer, 0, XER_SO_SHIFT | 1<<8);
        set_crb(ctx, 3, so);
        ctx->live.crb[2] = 0;
        ctx->crb_dirty &= ~(1u << 2);
        ctx->crb_loaded |= 0x80000000u >> 2;
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, flag, 0, skip_label);
        rtl_add_insn(unit, RTLOP_STORE_I8, 0, psb_reg, zero,
                     handle->setup.state_offset_reserve_flag);

        int old_value = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD, old_value, psb_reg, 0,
                     handle->setup.state_offset_reserve_state);
        if (handle->host_little_endian) {
            const int temp = old_value;
            old_value = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BSWAP, old_value, temp, 0, 0);
        }

        int new_value = get_gpr(ctx, insn_rS(insn));
        if (handle->host_little_endian) {
            const int temp = new_value;
            new_value = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BSWAP, new_value, temp, 0, 0);
        }
        const int host_address = get_ea_indexed(ctx, insn, NULL);
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_CMPXCHG,
                     result, host_address, old_value, new_value);
        const int success = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQ, success, result, old_value, 0);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, success, 0, skip_label);

        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, rtl_imm32(unit, 1), 0, ctx->alias.crb[2]);

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);

        return;
      }  // case XO_STWCX
      case XO_ECIWX:
      case XO_ECOWX:
      case XO_TLBSYNC:
        translate_unimplemented_insn(ctx, address, insn);
        return;
      case XO_LWBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_BR, false, true, false);
        return;
      case XO_SYNC:
      case XO_EIEIO:
        // FIXME: We currently act as if all loads and stores are sequential.
        return;
      case XO_STWBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_BR, true, true, false);
        return;
      case XO_LHBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16_BR,
                                 false, true, false);
        return;
      case XO_STHBRX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16_BR,
                                 true, true, false);
        return;
      case XO_ICBI:
        /* icbi implies that already-translated code may have changed, so
         * unconditionally return from this unit.  We currently don't
         * bother checking the invalidation address. */
        store_live_regs(ctx, true, true);
        set_nia_imm(ctx, address + 4);
        post_insn_callback(ctx, address);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, ctx->epilogue_label);
        return;
      case XO_DCBZ: {
        int addr32;
        if (insn_rA(insn)) {
            const int rA = get_gpr(ctx, insn_rA(insn));
            const int rB = get_gpr(ctx, insn_rB(insn));
            addr32 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ADD, addr32, rA, rB, 0);
        } else {
            addr32 = get_gpr(ctx, insn_rB(insn));
        }
        const int addr32_aligned = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, addr32_aligned, addr32, 0, -32);
        const int addr_aligned = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ZCAST, addr_aligned, addr32_aligned, 0, 0);
        const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_ADD,
                     host_address, ctx->membase_reg, addr_aligned, 0);
        // FIXME: use V2_FLOAT64 when we have a way to create one
        const int zero = rtl_imm64(unit, 0);
        for (int i = 0; i < 32; i += 8) {
            rtl_add_insn(unit, RTLOP_STORE, 0, host_address, zero, i);
        }
        return;
      }  // case XO_DCBZ

      /* XO_5 = 0x17 */
      case XO_LWZX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, true, false);
        return;
      case XO_LWZUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, true, true);
        return;
      case XO_LBZX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8, false, true, false);
        return;
      case XO_LBZUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8, false, true, true);
        return;
      case XO_STWX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, true, false);
        return;
      case XO_STWUX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, true, true);
        return;
      case XO_STBX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8, true, true, false);
        return;
      case XO_STBUX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8, true, true, true);
        return;
      case XO_LHZX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16, false, true, false);
        return;
      case XO_LHZUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16, false, true, true);
        return;
      case XO_LHAX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16, false, true, false);
        return;
      case XO_LHAUX:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16, false, true, true);
        return;
      case XO_STHX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16, true, true, false);
        return;
      case XO_STHUX:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16, true, true, true);
        return;
      case XO_LFSX:
      case XO_LFSUX:
      case XO_LFDX:
      case XO_LFDUX:
      case XO_STFSX:
      case XO_STFSUX:
      case XO_STFDX:
      case XO_STFDUX:
      case XO_STFIWX:
        return;  // FIXME: not yet implemented

      /* XO_5 = 0x18 */
      case XO_SLW:
        translate_shift(ctx, insn, RTLOP_SLL, false, false);
        return;
      case XO_SRW:
        translate_shift(ctx, insn, RTLOP_SRL, false, false);
        return;
      case XO_SRAW:
        translate_shift(ctx, insn, RTLOP_SRA, false, true);
        return;
      case XO_SRAWI:
        translate_shift(ctx, insn, RTLOP_SRAI, true, true);
        return;

      /* XO_5 = 0x1A */
      case XO_CNTLZW:
        translate_bitmisc(ctx, insn, RTLOP_CLZ);
        return;
      case XO_EXTSH:
        translate_bitmisc(ctx, insn, RTLOP_SEXT16);
        return;
      case XO_EXTSB:
        translate_bitmisc(ctx, insn, RTLOP_SEXT8);
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
        if (insn_rB(insn) == insn_rS(insn)) {  // mr rA,rS
            const int rS = get_gpr(ctx, insn_rS(insn));
            set_gpr(ctx, insn_rA(insn), rS);
            if (insn_Rc(insn)) {
                update_cr0(ctx, rS);
            }
        } else {
            translate_logic_reg(ctx, insn, RTLOP_OR, false, false);
        }
        return;
      case XO_NAND:
        translate_logic_reg(ctx, insn, RTLOP_AND, false, true);
        return;
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

    /* Skip instructions which were translated as part of an optimized
     * instruction pair (such as sc followed by blr). */
    if (ctx->skip_next_insn) {
        ctx->skip_next_insn = false;
        return;
    }

    if (UNLIKELY(!is_valid_insn(insn))) {
        rtl_add_insn(unit, RTLOP_ILLEGAL, 0, 0, 0, 0);
        return;
    }

    switch (insn_OPCD(insn)) {
      case OPCD_TWI:
        translate_trap(ctx, address, insn, rtl_imm32(unit, insn_SIMM(insn)));
        return;

      case OPCD_MULLI:
        translate_arith_imm(ctx, insn, RTLOP_MULI, false, false, false);
        return;

      case OPCD_SUBFIC: {
        const int rA = get_gpr(ctx, insn_rA(insn));
        const int imm = rtl_imm32(unit, insn_SIMM(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, imm, rA, 0);
        set_gpr(ctx, insn_rD(insn), result);

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
        if (insn_rA(insn) == 0) {  // li
            set_gpr(ctx, insn_rD(insn), rtl_imm32(unit, insn_SIMM(insn)));
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, false, false, false);
        }
        return;

      case OPCD_ADDIS:
        if (insn_rA(insn) == 0) {  // lis
            set_gpr(ctx, insn_rD(insn), rtl_imm32(unit, insn_SIMM(insn) << 16));
        } else {
            translate_arith_imm(ctx, insn, RTLOP_ADDI, true, false, false);
        }
        return;

      case OPCD_BC:
        translate_branch(ctx, address, insn_BO(insn), insn_BI(insn),
                         insn_BD(insn), insn_AA(insn), insn_LK(insn));
        return;

      case OPCD_SC: {
        /* Special case: translate sc followed by blr in a single step, to
         * avoid having to return to caller and call a new unit containing
         * just the blr.  The scanner will terminate the block at an sc
         * instruction which is not followed by a blr, so we only need to
         * check whether this sc is at the end of the block. */
        bool is_sc_blr = false;
        if (address + 4 < block->start + block->len) {
            ASSERT(address + 8 == block->start + block->len);
            const uint32_t *memory_base =
                (const uint32_t *)ctx->handle->setup.guest_memory_base;
            const uint32_t next_insn = bswap_be32(memory_base[(address+4)/4]);
            ASSERT(next_insn == 0x4E800020);
            is_sc_blr = true;
        }
        int nia;
        if (is_sc_blr) {
            const int lr = get_lr(ctx);
            nia = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, nia, lr, 0, -4);
        } else {
            nia = rtl_imm32(unit, address + 4);
        }
        guest_ppc_flush_cr(ctx, true);
        store_live_regs(ctx, true, true);
        set_nia(ctx, nia);
        const int sc_handler = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
        rtl_add_insn(unit, RTLOP_LOAD, sc_handler, ctx->psb_reg, 0,
                     ctx->handle->setup.state_offset_sc_handler);
        rtl_add_insn(unit, RTLOP_CALL, 0, sc_handler, ctx->psb_reg, 0);
        post_insn_callback(ctx, address);
        rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0);
        ctx->skip_next_insn = is_sc_blr;
        return;
      }  // case OPCD_SC

      case OPCD_B:
        translate_branch(ctx, address, 0x14, 0, insn_LI(insn),
                         insn_AA(insn), insn_LK(insn));
        return;

      case OPCD_x13:
        switch ((PPCExtendedOpcode13)insn_XO_10(insn)) {
          case XO_MCRF:
            for (int i = 0; i < 4; i++) {
                set_crb(ctx, insn_crfD(insn)*4 + i,
                        get_crb(ctx, insn_crfS(insn)*4 + i));
            }
            return;
          case XO_BCLR:
            translate_branch_terminal(ctx, address, insn_BO(insn),
                                      insn_BI(insn), insn_LK(insn),
                                      get_lr(ctx), true);
            return;
          case XO_CRNOR:
            /* "crnor crbD,crbA,crbB" is also known as "crnot crbD,crbA",
             * but the usage frequency of crnot is probably not very high
             * in typical code and the potential speed gain is minimal, so
             * we don't bother with special handling. */
            translate_logic_crb(ctx, insn, RTLOP_OR, false, true);
            return;
          case XO_RFI:
            translate_unimplemented_insn(ctx, address, insn);
            return;
          case XO_CRANDC:
            translate_logic_crb(ctx, insn, RTLOP_AND, true, false);
            return;
          case XO_ISYNC:
            // FIXME: We currently act as if all instructions are sequential.
            return;
          case XO_CRXOR:
            if (insn_crbA(insn) == insn_crbB(insn)) {  // crclr
                set_crb(ctx, insn_crbD(insn), rtl_imm32(unit,0));
            } else {
                translate_logic_crb(ctx, insn, RTLOP_XOR, false, false);
            }
            return;
          case XO_CRNAND:
            translate_logic_crb(ctx, insn, RTLOP_AND, false, true);
            return;
          case XO_CRAND:
            translate_logic_crb(ctx, insn, RTLOP_AND, false, false);
            return;
          case XO_CREQV:
            if (insn_crbA(insn) == insn_crbB(insn)) {  // crset
                set_crb(ctx, insn_crbD(insn), rtl_imm32(unit,1));
            } else {
                /* See note at XO_EQV under opcode 0x1F (though it's less
                 * likely to help in this case). */
                translate_logic_crb(ctx, insn, RTLOP_XOR, true, false);
            }
            return;
          case XO_CRORC:
            translate_logic_crb(ctx, insn, RTLOP_OR, true, false);
            return;
          case XO_CROR:
            /* "cror crbD,crbA,crbB" is also known as "crmove crbD,crbA",
             * but the usage frequency of crmove is probably not very high
             * in typical code and the potential speed gain is minimal, so
             * we don't bother with special handling. */
            translate_logic_crb(ctx, insn, RTLOP_OR, false, false);
            return;
          case XO_BCCTR:
            translate_branch_terminal(ctx, address, insn_BO(insn),
                                      insn_BI(insn), insn_LK(insn),
                                      get_ctr(ctx), true);
            return;
        }
        ASSERT(!"Missing 0x13 extended opcode handler");

      case OPCD_RLWIMI:
        translate_rotate_mask(ctx, insn, true, true);
        return;

      case OPCD_RLWINM:
        if ((get_insn_at(ctx, block, address-4) & 0xFC1F03FE)
            == (OPCD_x1F<<26 | insn_rS(insn)<<16 | XO_CNTLZW<<1)
         && insn_SH(insn) == 27
         && insn_MB(insn) == 5
         && insn_ME(insn) == 31) {
            /* "cntlzw temp,rX; srwi rY,temp,5" is a common PowerPC idiom
             * for comparing a value to zero and getting the result as an
             * integer rather than a condition flag.  We leave the cntlzw
             * in place in case its result happens to also be used
             * elsewhere; dead store elimination will remove it if not. */
            const int cntlzw_rS = insn_rS(get_insn_at(ctx, block, address-4));
            const int rlwinm_rA = insn_rA(insn);
            const int value = get_gpr(ctx, cntlzw_rS);
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI, result, value, 0, 0);
            set_gpr(ctx, rlwinm_rA, result);
            if (insn_Rc(insn)) {
                const int lt = rtl_imm32(unit, 0);
                const int gt = result;
                const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_XORI, eq, result, 0, 1);
                const int xer = get_xer(ctx);
                const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT,
                             so, xer, 0, XER_SO_SHIFT | 1<<8);
                set_crb(ctx, 0, lt);
                set_crb(ctx, 1, gt);
                set_crb(ctx, 2, eq);
                set_crb(ctx, 3, so);
            }
            return;
        }
        translate_rotate_mask(ctx, insn, true, false);
        return;

      case OPCD_RLWNM:
        translate_rotate_mask(ctx, insn, false, false);
        return;

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
        translate_x1F(ctx, address, insn);
        return;

      case OPCD_LWZ:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, false, false);
        return;

      case OPCD_LWZU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD, false, false, true);
        return;

      case OPCD_LBZ:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8,
                                 false, false, false);
        return;

      case OPCD_LBZU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U8,
                                 false, false, true);
        return;

      case OPCD_STW:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, false, false);
        return;

      case OPCD_STWU:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE, true, false, true);
        return;

      case OPCD_STB:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8,
                                 true, false, false);
        return;

      case OPCD_STBU:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I8,
                                 true, false, true);
        return;

      case OPCD_LHZ:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16,
                                 false, false, false);
        return;

      case OPCD_LHZU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_U16,
                                 false, false, true);
        return;

      case OPCD_LHA:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16,
                                 false, false, false);
        return;

      case OPCD_LHAU:
        translate_load_store_gpr(ctx, insn, RTLOP_LOAD_S16,
                                 false, false, true);
        return;

      case OPCD_STH:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16,
                                 true, false, false);
        return;

      case OPCD_STHU:
        translate_load_store_gpr(ctx, insn, RTLOP_STORE_I16,
                                 true, false, true);
        return;

      case OPCD_LMW:
        translate_load_store_multiple(ctx, insn, false);
        return;

      case OPCD_STMW:
        translate_load_store_multiple(ctx, insn, true);
        return;

      default: return;  // FIXME: not yet implemented
    }  // switch (insn_OPCD(insn))

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
        (const uint32_t *)ctx->handle->setup.guest_memory_base;

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, block->label);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to add label at 0x%X", start);
        return false;
    }

    if (UNLIKELY(block->len == 0)) {
        /* This block was a backward branch target that wasn't part of a
         * previous block (see block-splitting logic at the bottom of
         * guest_ppc_scan()).  Update NIA and return to the caller to
         * retranslate from the target address. */
        set_nia_imm(ctx, start);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, ctx->epilogue_label);
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate empty block at 0x%X",
                    start);
            return false;
        }
        return true;
    }

    memset(&ctx->live, 0, sizeof(ctx->live));
    ctx->gpr_dirty = 0;
    ctx->fpr_dirty = 0;
    ctx->crb_dirty = 0;
    ctx->lr_dirty = 0;
    ctx->ctr_dirty = 0;
    ctx->xer_dirty = 0;
    ctx->fpscr_dirty = 0;

    ctx->skip_next_insn = false;

    for (uint32_t ofs = 0; ofs < block->len; ofs += 4) {
        const uint32_t address = start + ofs;
        if (ctx->handle->pre_insn_callback) {
            store_live_regs(ctx, false, false);
            guest_ppc_flush_cr(ctx, false);
            const int func = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_LOAD_IMM, func, 0, 0,
                         (uintptr_t)ctx->handle->pre_insn_callback);
            rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT,
                         0, func, ctx->psb_reg, rtl_imm32(unit, address));
        }

        const uint32_t insn = bswap_be32(memory_base[address/4]);
        translate_insn(ctx, block, address, insn);

        /* Explicitly check for the presence of a callback (even though
         * post_insn_callback() does so too) so we don't repeatedly set
         * NIA if it's not necessary. */
        if (ctx->handle->post_insn_callback) {
            set_nia_imm(ctx, address + 4);
            post_insn_callback(ctx, address);
        }

        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate instruction at 0x%X",
                    address);
            return false;
        }
    }

    store_live_regs(ctx, true, true);
    set_nia_imm(ctx, start + block->len);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to update registers after block end 0x%X",
                start + block->len - 4);
        return false;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

void guest_ppc_flush_cr(GuestPPCContext *ctx, bool make_live)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    RTLUnit * const unit = ctx->unit;

    if (ctx->crb_loaded & ctx->crb_changed) {
        const int cr = merge_cr(ctx, make_live);
        if (make_live) {
            set_cr(ctx, cr);
            memset(ctx->live.crb, 0, sizeof(ctx->live.crb));
            ctx->crb_loaded = 0;
            ctx->crb_dirty = 0;
        } else {
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, cr, 0, ctx->alias.cr);
        }
    }
}

/*************************************************************************/
/*************************************************************************/
