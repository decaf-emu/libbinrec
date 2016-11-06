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
#include "src/rtl-internal.h"

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
 * if the given address is outside the current block.  The address is
 * assumed to be properly aligned.
 */
static inline PURE_FUNCTION uint32_t get_insn_at(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address)
{
    ASSERT((address & 3) == 0);
    if (address - block->start < block->len) {
        const uint32_t *memory_base =
            (const uint32_t *)ctx->handle->setup.guest_memory_base;
        return bswap_be32(memory_base[address/4]);
    } else {
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * convert_fpr:  Convert a floating-point value from one type to another,
 * and return an RTL register containing the converted value.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: FPR index.
 *     reg: RTL register containing FPR's value.
 *     old_type: Type of "reg".
 *     new_type: Type to convert value to.  Must be different from old_type.
 * [Return value]
 *     Index of RTL register containing converted value.
 */
static inline int convert_fpr(GuestPPCContext *ctx, int index, int reg,
                              RTLDataType old_type, RTLDataType new_type)
{
    ASSERT(old_type != new_type);
    ASSERT(rtl_type_is_float(old_type)
           || (rtl_type_is_vector(old_type) && rtl_vector_length(old_type) == 2
               && rtl_type_is_float(rtl_vector_element_type(old_type))));
    ASSERT(rtl_type_is_float(new_type)
           || (rtl_type_is_vector(new_type) && rtl_vector_length(new_type) == 2
               && rtl_type_is_float(rtl_vector_element_type(new_type))));

    RTLUnit * const unit = ctx->unit;

    if (old_type == RTLTYPE_V2_FLOAT64) {
        if (new_type == RTLTYPE_V2_FLOAT32) {
            const int new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VFCAST, new_reg, reg, 0, 0);
            return new_reg;
        } else {
            const int f64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_VEXTRACT, f64, reg, 0, 0);
            if (new_type == RTLTYPE_FLOAT64) {
                return f64;
            } else {
                ASSERT(new_type == RTLTYPE_FLOAT32);
                const int new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
                rtl_add_insn(unit, RTLOP_FCAST, new_reg, f64, 0, 0);
                return new_reg;
            }
        }

    } else if (old_type == RTLTYPE_V2_FLOAT32) {
        if (new_type == RTLTYPE_FLOAT32) {
            const int new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_VEXTRACT, new_reg, reg, 0, 0);
            return new_reg;
        } else {
            const int f64x2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_VFCAST, f64x2, reg, 0, 0);
            if (new_type == RTLTYPE_V2_FLOAT64) {
                return f64x2;
            } else {
                ASSERT(new_type == RTLTYPE_FLOAT64);
                ASSERT(unit->aliases[ctx->alias.fpr[index]].type
                       == RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, f64x2, 0, ctx->alias.fpr[index]);
                const int new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
                rtl_add_insn(unit, RTLOP_VEXTRACT, new_reg, f64x2, 0, 0);
                return new_reg;
            }
        }

    } else if (old_type == RTLTYPE_FLOAT64) {
        if (new_type == RTLTYPE_FLOAT32) {
            const int new_reg = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_FCAST, new_reg, reg, 0, 0);
            return new_reg;
        } else {
            ASSERT(unit->aliases[ctx->alias.fpr[index]].type
                   == RTLTYPE_V2_FLOAT64);
            const int old_f64x2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         old_f64x2, 0, 0, ctx->alias.fpr[index]);
            const int f64x2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
            rtl_add_insn(unit, RTLOP_VINSERT, f64x2, old_f64x2, reg, 0);
            if (new_type == RTLTYPE_V2_FLOAT64) {
                return f64x2;
            } else {
                ASSERT(new_type == RTLTYPE_V2_FLOAT32);
                const int new_reg = rtl_alloc_register(unit,
                                                       RTLTYPE_V2_FLOAT32);
                rtl_add_insn(unit, RTLOP_VFCAST, new_reg, f64x2, 0, 0);
                return new_reg;
            }
        }

    } else {
        ASSERT(old_type == RTLTYPE_FLOAT32);
        if (new_type == RTLTYPE_V2_FLOAT32) {
            const int new_reg = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32);
            rtl_add_insn(unit, RTLOP_VBROADCAST, new_reg, reg, 0, 0);
            return new_reg;
        } else {
            const int f64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_FCAST, f64, reg, 0, 0);
            if (new_type == RTLTYPE_V2_FLOAT64) {
                const int new_reg = rtl_alloc_register(unit,
                                                       RTLTYPE_V2_FLOAT64);
                rtl_add_insn(unit, RTLOP_VBROADCAST, new_reg, f64, 0, 0);
                return new_reg;
            } else {
                ASSERT(new_type == RTLTYPE_FLOAT64);
                if (!(ctx->fpr_is_ps & (1 << index))) {
                    rtl_add_insn(unit, RTLOP_STORE, 0, ctx->psb_reg, f64,
                                 (ctx->handle->setup.state_offset_fpr
                                  + 16*index + 8));
                }
                return f64;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_gpr, get_fpr, get_cr, get_crb, get_lr, get_ctr, get_xer, get_fpscr,
 * get_fr_fi_fprf:  Return an RTL register containing the value of the
 * given PowerPC register.  This will either be the register last used in
 * a corresponding set or get operation, or a newly allocated register (in
 * which case an appropriate GET_ALIAS instruction will also be added).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (get_gpr(), get_fpr()) or CR bit index
 *         (get_crb()).
 *     type: Floating-point data type (RTLTYPE_*) (get_fpr() only).
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

static inline int get_fpr(GuestPPCContext * const ctx, int index,
                          RTLDataType type)
{
    int reg;
    if (ctx->live.fpr[index]) {
        reg = ctx->live.fpr[index];
    } else {
        const RTLDataType base_type = (ctx->fpr_is_ps & (1 << index)
                                       ? RTLTYPE_V2_FLOAT64 : RTLTYPE_FLOAT64);
        RTLUnit * const unit = ctx->unit;
        reg = rtl_alloc_register(unit, base_type);
        ASSERT(ctx->alias.fpr[index]);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fpr[index]);
        ctx->fpr_type[index] = base_type;
    }
    ASSERT(ctx->fpr_type[index]);
    if (type != ctx->fpr_type[index]) {
        reg = convert_fpr(ctx, index, reg, ctx->fpr_type[index], type);
        ctx->fpr_type[index] = type;
    }
    return reg;
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
        if (ctx->crb_loaded & (0x80000000u >> index)) {
            ASSERT(ctx->alias.crb[index]);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.crb[index]);
        } else {
            const int cr = get_cr(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT, reg, cr, 0, (31-index) | (1<<8));
            if (ctx->crb_changed & (0x80000000u >> index)) {
                ASSERT(ctx->alias.crb[index]);
                rtl_add_insn(unit, RTLOP_SET_ALIAS,
                             0, reg, 0, ctx->alias.crb[index]);
                ctx->crb_loaded |= 0x80000000u >> index;
            }
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

static inline int get_fr_fi_fprf(GuestPPCContext * const ctx)
{
    if (ctx->live.fr_fi_fprf) {
        return ctx->live.fr_fi_fprf;
    } else {
        RTLUnit * const unit = ctx->unit;
        const int reg = rtl_alloc_register(unit, RTLTYPE_INT32);
        ASSERT(ctx->alias.fr_fi_fprf);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, ctx->alias.fr_fi_fprf);
        ctx->live.fr_fi_fprf = reg;
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
        if (ctx->crb_loaded & (0x80000000u >> index)) {
            ASSERT(ctx->alias.crb[index]);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_GET_ALIAS,
                         reg, 0, 0, ctx->alias.crb[index]);
            ctx->live.crb[index] = reg;
        } else {
            const int cr = get_cr(ctx);
            reg = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, reg, cr, 0, 0x80000000u >> index);
        }
        return reg;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * set_gpr, set_fpr, set_cr, set_crb, set_lr, set_ctr, set_xer, set_fpscr,
 * set_fr_fi_fprf:  Store the given RTL register to the given PowerPC
 * register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: PowerPC register index (set_gpr(), set_fpr()) or CR bit index
 *         (set_crb()).
 *     reg: Register to store.
 */
static inline void set_gpr(GuestPPCContext * const ctx, int index, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.gpr[index] >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.gpr[index], false);
    }
    ctx->last_set.gpr[index] = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.gpr[index]);
    ctx->live.gpr[index] = reg;
}

static inline void set_fpr(GuestPPCContext * const ctx, int index, int reg)
{
    ctx->live.fpr[index] = reg;
    ctx->fpr_type[index] = ctx->unit->regs[reg].type;
}

static inline void set_cr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.cr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.cr, false);
    }
    ctx->last_set.cr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.cr);
    ctx->live.cr = reg;
}

static inline void set_crb(GuestPPCContext * const ctx, int index, int reg)
{
    RTLUnit * const unit = ctx->unit;
    ASSERT(ctx->crb_changed & (0x80000000u >> index));
    ctx->crb_loaded |= 0x80000000u >> index;
    if (ctx->last_set.crb[index] >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.crb[index], false);
    }
    ctx->last_set.crb[index] = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.crb[index]);
    ctx->live.crb[index] = reg;
    ctx->crb_dirty |= 1u << index;
}

static inline void set_lr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.lr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.lr, false);
    }
    ctx->last_set.lr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.lr);
    ctx->live.lr = reg;
}

static inline void set_ctr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.ctr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.ctr, false);
    }
    ctx->last_set.ctr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.ctr);
    ctx->live.ctr = reg;
}

static inline void set_xer(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.xer >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.xer, false);
    }
    ctx->last_set.xer = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.xer);
    ctx->live.xer = reg;
}

static inline void set_fpscr(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.fpscr >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.fpscr, false);
    }
    ctx->last_set.fpscr = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.fpscr);
    ctx->live.fpscr = reg;
}

static inline void set_fr_fi_fprf(GuestPPCContext * const ctx, int reg)
{
    RTLUnit * const unit = ctx->unit;
    if (ctx->last_set.fr_fi_fprf >= 0) {
        rtl_opt_kill_insn(unit, ctx->last_set.fr_fi_fprf, false);
    }
    ctx->last_set.fr_fi_fprf = unit->num_insns;
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, ctx->alias.fr_fi_fprf);
    ctx->live.fr_fi_fprf = reg;
}

/*-----------------------------------------------------------------------*/

/**
 * get_fpscr_fex_vx:  Return RTL registers containing the FEX and VX bits
 * for the given value of FPSCR.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     fpscr: RTL register containing the FPSCR value.
 *     fex_ret: Pointer to variable to receive an RTL register containing
 *        the value of the FEX bit.
 *     vx_ret: Pointer to variable to receive an RTL register containing
 *        the value of the VX bit.
 */
static void get_fpscr_fex_vx(GuestPPCContext *ctx, int fpscr,
                             int *fex_ret, int *vx_ret)
{
    ASSERT(fex_ret);
    ASSERT(vx_ret);

    RTLUnit * const unit = ctx->unit;

    const int vx_test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, vx_test, fpscr, 0, FPSCR_ALL_VXFOO);
    const int vx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTUI, vx, vx_test, 0, 0);
    *vx_ret = vx;

    const int x_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, x_bits, fpscr, 0, FPSCR_XX_SHIFT | 4<<8);
    const int vx_shifted = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, vx_shifted, vx, 0, 4);
    const int e_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SRLI, e_bits, fpscr, 0, FPSCR_XE_SHIFT);
    const int x_bits_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, x_bits_vx, x_bits, vx_shifted, 0);
    const int both_bits = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_AND, both_bits, x_bits_vx, e_bits, 0);
    const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, test, both_bits, 0, 31);
    const int fex = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SGTUI, fex, test, 0, 0);
    *fex_ret = fex;
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
 * gen_load_store_address:  Generate the address for a load or store operation.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_indexed: True if the access is an indexed access, false if not.
 *     update: True if rA will be updated, false if not.
 *     disp_ret: Pointer to variable to receive the access offset for the
 *         load/store instruction.  Always receives zero if is_indexed or
 *         update is true.
 *     ea_ret: Pointer to variable to receive the effective address to
 *         write back to rA for an update operation.  Not modified (and may
 *         be NULL) if update is false.
 * [Return value]
 *     RTL register containing the host address for the load/store instruction.
 */
static int gen_load_store_address(GuestPPCContext *ctx, uint32_t insn,
                                  bool is_indexed, bool update,
                                  int *disp_ret, int *ea_ret)
{
    ASSERT(ctx);
    ASSERT(disp_ret);
    ASSERT(!update || ea_ret);

    RTLUnit * const unit = ctx->unit;
    int host_address;

    if (update) {
        ASSERT(insn_rA(insn) != 0);
        if (is_indexed) {
            host_address = get_ea_indexed(ctx, insn, ea_ret);
        } else {
            int ea;
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
            *ea_ret = ea;
        }
        *disp_ret = 0;
    } else {
        if (is_indexed) {
            host_address = get_ea_indexed(ctx, insn, NULL);
            *disp_ret = 0;
        } else {
            if (insn_rA(insn) != 0 || insn_d(insn) >= 0) {
                host_address = get_ea_base(ctx, insn);
                *disp_ret = insn_d(insn);
            } else {
                const int offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
                rtl_add_insn(unit, RTLOP_LOAD_IMM,
                             offset, 0, 0, (uint32_t)insn_d(insn));
                host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
                rtl_add_insn(unit, RTLOP_ADD,
                             host_address, ctx->membase_reg, offset, 0);
                *disp_ret = 0;
            }
        }
    }

    return host_address;
}

/*-----------------------------------------------------------------------*/

/**
 * flush_live_regs:  Finalize all pending stores of guest registers.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     clear: True to clear all live registers after flushing them; false
 *         to leave them live.
 */
static void flush_live_regs(GuestPPCContext *ctx, bool clear)
{
    for (int i = 0; i < 32; i++) {
        int reg = ctx->live.fpr[i];
        if (reg) {
            const RTLDataType base_type = (ctx->fpr_is_ps & (1 << i)
                                           ? RTLTYPE_V2_FLOAT64
                                           : RTLTYPE_FLOAT64);
            if (ctx->fpr_type[i] != base_type) {
                reg = convert_fpr(ctx, i, reg, ctx->fpr_type[i], base_type);
            }
            rtl_add_insn(ctx->unit, RTLOP_SET_ALIAS,
                         0, reg, 0, ctx->alias.fpr[i]);
        }
    }

    memset(&ctx->last_set, -1, sizeof(ctx->last_set));
    if (clear) {
        memset(&ctx->live, 0, sizeof(ctx->live));
        memset(ctx->fpr_type, 0, sizeof(ctx->fpr_type));
        ctx->crb_dirty = 0;
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

    uint32_t crb_loaded = ctx->crb_loaded;
    ASSERT(crb_loaded != 0);  // We won't be called if there's nothing to merge.

    int cr;
    if (crb_loaded == ~UINT32_C(0)) {
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
        rtl_add_insn(unit, RTLOP_ANDI, cr, old_cr, 0, ~crb_loaded);
    }

    while (crb_loaded) {
        const int bit = clz32(crb_loaded);
        crb_loaded ^= 0x80000000u >> bit;
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
 * merge_fpscr:  Merge the FR/FI/FPRF alias into FPSCR and return an RTL
 * register containing the merged value.  Helper for guest_ppc_flush_fpscr().
 *
 * [Parameters]
 *     ctx: Translation context.
 *     make_live: True to always leave FPSCR live in its alias, false to not
 *         call get_fpscr() if FPSCR is not live.
 * [Return value]
 *     RTL register containing merged value of FPSCR.
 */
static int merge_fpscr(GuestPPCContext *ctx, bool make_live)
{
    RTLUnit * const unit = ctx->unit;

    int fpscr;
    if (make_live) {
        fpscr = get_fpscr(ctx);
    } else if (ctx->live.fpscr) {
        fpscr = ctx->live.fpscr;
    } else {
        fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, fpscr, 0, 0, ctx->alias.fpscr);
    }
    const int fr_fi_fprf = get_fr_fi_fprf(ctx);

    const int masked_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, masked_fpscr, fpscr, 0,
                 ~(FPSCR_FEX | FPSCR_VX | FPSCR_FR | FPSCR_FI | FPSCR_FPRF
                   | FPSCR_RESV20));
    const int shifted_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI,
                 shifted_fprf, fr_fi_fprf, 0, FPSCR_FPRF_SHIFT);

    const int merged_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, merged_fpscr, masked_fpscr, shifted_fprf, 0);

    return merged_fpscr;
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
        flush_live_regs(ctx, false);
        guest_ppc_flush_cr(ctx, false);
        guest_ppc_flush_fpscr(ctx, false);
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

/*-----------------------------------------------------------------------*/

/**
 * update_cr1:  Add RTL instructions to update the value of CR1 based on
 * the high 4 bits of FPSCR.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
static void update_cr1(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    const int fpscr = get_fpscr(ctx);
    int fex, vx;
    get_fpscr_fex_vx(ctx, fpscr, &fex, &vx);
    const int fx = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, fx, fpscr, 0, 31 | 1<<8);
    const int ox = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_BFEXT, ox, fpscr, 0, 28 | 1<<8);
    set_crb(ctx, 4, fx);
    set_crb(ctx, 5, fex);
    set_crb(ctx, 6, vx);
    set_crb(ctx, 7, ox);
}

/*-----------------------------------------------------------------------*/

/**
 * update_rounding_mode:  Add RTL instructions to update the current
 * floating-point rounding mode based on the RN field of FPSCR.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
static void update_rounding_mode(GuestPPCContext *ctx)
{
    RTLUnit * const unit = ctx->unit;

    const int label_n = rtl_alloc_label(unit);
    const int label_z = rtl_alloc_label(unit);
    const int label_p = rtl_alloc_label(unit);
    const int label_out = rtl_alloc_label(unit);

    const int fpscr = get_fpscr(ctx);
    const int rn = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, rn, fpscr, 0, FPSCR_RN);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, rn, 0, label_n);
    const int test_1 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLTUI, test_1, rn, 0, 2);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, test_1, 0, label_z);
    const int test_2 = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SEQI, test_2, rn, 0, 2);
    rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, test_2, 0, label_p);

    rtl_add_insn(unit, RTLOP_FSETROUND, 0, 0, 0, RTLFROUND_FLOOR);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_p);
    rtl_add_insn(unit, RTLOP_FSETROUND, 0, 0, 0, RTLFROUND_CEIL);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_z);
    rtl_add_insn(unit, RTLOP_FSETROUND, 0, 0, 0, RTLFROUND_TRUNC);
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label_out);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_n);
    rtl_add_insn(unit, RTLOP_FSETROUND, 0, 0, 0, RTLFROUND_NEAREST);

    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_out);
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
static void translate_arith_imm(
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
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, ~XER_OV);
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
static void translate_bitmisc(
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

    const bool is_conditional = ((BO & 0x14) != 0x14);

    int skip_label;
    if ((BO & 0x14) == 0 || (is_conditional && handle->use_branch_callback)) {
        /* Need an extra label in case a non-final test fails. */
        skip_label = rtl_alloc_label(unit);
    } else {
        skip_label = 0;
    }

    RTLOpcode branch_op = RTLOP_GOTO;
    int test_reg = 0;

    uint32_t crb_store_branch = 0;
    uint32_t crb_store_next = 0;
    uint16_t crb_reg[32];
    RTLInsn crb_insn[32];  // Copy of the instruction that sets each value.
    if (ctx->handle->guest_opt & BINREC_OPT_G_PPC_TRIM_CR_STORES) {
        guest_ppc_trim_cr_stores(ctx, BO, BI, &crb_store_branch,
                                 &crb_store_next, crb_reg, crb_insn);
        /* If there are bits to store on the branch-taken path for a
         * conditional branch, we need a label for skipping past the branch
         * even for a single condition (much like when the branch callback
         * is enabled). */
        if (crb_store_branch && !skip_label) {
            skip_label = rtl_alloc_label(unit);
        }
    }

    if (!(BO & 0x04)) {
        const int ctr = get_ctr(ctx);
        const int new_ctr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ADDI, new_ctr, ctr, 0, -1);
        set_ctr(ctx, new_ctr);

        /* Flush here so any update to CTR is stored along with other
         * pending changes. */
        flush_live_regs(ctx, false);

        if (skip_label) {
            rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                         0, new_ctr, 0, skip_label);
        } else {
            branch_op = BO & 0x02 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ;
            test_reg = new_ctr;
        }
    } else {
        flush_live_regs(ctx, false);
    }
    /* All dirty registers have been flushed at this point. */

    if (!(BO & 0x10)) {
        const int test = test_crb(ctx, BI);
        if (handle->use_branch_callback || crb_store_branch) {
            rtl_add_insn(unit, BO & 0x08 ? RTLOP_GOTO_IF_Z : RTLOP_GOTO_IF_NZ,
                         0, test, 0, skip_label);
        } else {
            branch_op = BO & 0x08 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z;
            test_reg = test;
        }
    }

    while (crb_store_branch) {
        const int bit = ctz32(crb_store_branch);
        crb_store_branch ^= 1u << bit;
        if (crb_insn[bit].opcode) {
            rtl_add_insn_copy(unit, &crb_insn[bit]);
        }
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, crb_reg[bit], 0, ctx->alias.crb[bit]);
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
                     0, callback_result, 0, guest_ppc_get_epilogue_label(ctx));
    }

    rtl_add_insn(unit, branch_op, 0, test_reg, 0, target_label);

    if (skip_label) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, skip_label);
    }

    while (crb_store_next) {
        const int bit = ctz32(crb_store_next);
        crb_store_next ^= 1u << bit;
        if (crb_insn[bit].opcode) {
            rtl_add_insn_copy(unit, &crb_insn[bit]);
        }
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, crb_reg[bit], 0, ctx->alias.crb[bit]);
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

        flush_live_regs(ctx, false);

        rtl_add_insn(unit, BO & 0x02 ? RTLOP_GOTO_IF_NZ : RTLOP_GOTO_IF_Z,
                     0, new_ctr, 0, skip_label);
    } else {
        flush_live_regs(ctx, false);
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
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, guest_ppc_get_epilogue_label(ctx));

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
static void translate_branch(
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
 * translate_compare:  Translate an integer compare instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_imm: True for a register-immediate compare (D-form instruction),
 *         false for a register-register compare (X-form instruction).
 *     is_signed: True for a signed compare, false for an unsigned compare.
 */
static void translate_compare(
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
        const int rB = get_gpr(ctx, insn_rB(insn));
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
 * translate_compare_fp:  Translate a floating-point compare instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     ordered: True for an ordered comparison (invalid exception on QNaN),
 *         false for an unordered comparison.
 */
static void translate_compare_fp(
    GuestPPCContext *ctx, uint32_t insn, bool ordered)
{
    RTLUnit * const unit = ctx->unit;

    const int obit = ordered ? RTLFCMP_ORDERED : 0;
    const int frA = get_fpr(ctx, insn_frA(insn), RTLTYPE_FLOAT64);
    const int frB = get_fpr(ctx, insn_frB(insn), RTLTYPE_FLOAT64);

    const int lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, lt, frA, frB, obit | RTLFCMP_LT);
    const int gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, gt, frA, frB, obit | RTLFCMP_GT);
    const int eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, eq, frA, frB, obit | RTLFCMP_EQ);
    const int un = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FCMP, un, frA, frB, obit | RTLFCMP_UN);

    set_crb(ctx, insn_crfD(insn)*4+0, lt);
    set_crb(ctx, insn_crfD(insn)*4+1, gt);
    set_crb(ctx, insn_crfD(insn)*4+2, eq);
    set_crb(ctx, insn_crfD(insn)*4+3, un);

    const int fr_fi_fprf = get_fr_fi_fprf(ctx);
    const int masked = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, masked, fr_fi_fprf, 0, 0x70);
    const int shifted_lt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_lt, lt, 0, 3);
    const int shifted_gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_gt, gt, 0, 2);
    const int shifted_eq = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_SLLI, shifted_eq, eq, 0, 1);
    const int lt_gt = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, lt_gt, shifted_lt, shifted_gt, 0);
    const int eq_un = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, eq_un, shifted_eq, un, 0);
    const int fpcc = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, fpcc, lt_gt, eq_un, 0);
    const int merged = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_OR, merged, masked, fpcc, 0);
    set_fr_fi_fprf(ctx, merged);

    const int fpstate = rtl_alloc_register(unit, RTLTYPE_FPSTATE);
    rtl_add_insn(unit, RTLOP_FGETSTATE, fpstate, 0, 0, 0);
    const int invalid = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_FTESTEXC, invalid, fpstate, 0, RTLFEXC_INVALID);
    const int label = rtl_alloc_label(unit);
    rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, invalid, 0, label);
    rtl_add_insn(unit, RTLOP_FCLEAREXC, 0, 0, 0, 0);
    const int fpscr = get_fpscr(ctx);
    /* FPSCR is changed conditionally, so we can't save it. */
    ctx->live.fpscr = 0;
    ctx->last_set.fpscr = -1;
    if (ordered && !(ctx->handle->guest_opt
                     & BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO)) {
        /* If both values have maximum exponent and high mantissa bit set,
         * they're both QNaNs and thus we should not set VXSNAN. */
        const int frA_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, frA_bits, frA, 0, 0);
        const int frB_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, frB_bits, frB, 0, 0);
        const int both_bits = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_AND, both_bits, frA_bits, frB_bits, 0);
        const int temp1 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_SRLI, temp1, both_bits, 0, 51);
        const int temp2 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, temp2, temp1, 0, 0);
        const int temp3 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ANDI, temp3, temp2, 0, 0xFFF);
        const int test = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SEQI, test, temp3, 0, 0xFFF);
        const int label_snan = rtl_alloc_label(unit);
        rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, test, 0, label_snan);
        const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0, FPSCR_VXVC);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_fpscr, 0, ctx->alias.fpscr);
        rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label);
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label_snan);
    }
    const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0,
                 ordered ? (FPSCR_VXSNAN | FPSCR_VXVC) : FPSCR_VXSNAN);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_fpscr, 0, ctx->alias.fpscr);
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_dcbz:  Translate a dcbz or dcbz_l instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 */
static void translate_dcbz(GuestPPCContext *ctx, uint32_t insn)
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
    const int addr32_aligned = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_ANDI, addr32_aligned, addr32, 0, -32);
    const int addr_aligned = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ZCAST, addr_aligned, addr32_aligned, 0, 0);
    const int host_address = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
    rtl_add_insn(unit, RTLOP_ADD,
                 host_address, ctx->membase_reg, addr_aligned, 0);

    const int zero_64 = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, zero_64, 0, 0, 0);
    const int zero_128 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64);
    rtl_add_insn(unit, RTLOP_VBROADCAST, zero_128, zero_64, 0, 0);
    rtl_add_insn(unit, RTLOP_STORE, 0, host_address, zero_128, 0);
    rtl_add_insn(unit, RTLOP_STORE, 0, host_address, zero_128, 16);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_load_store_fpr:  Translate a floating-point load or store
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 *     is_single: True if a single-precision (32-bit) operation, false if a
 *       double-precision (64-bit) operation.
 *     is_store: True if the instruction is a store instruction.
 *     is_indexed: True if the access is an indexed access (like lwx or stwx).
 *     update: True if register rA should be updated with the final EA.
 */
static void translate_load_store_fpr(
    GuestPPCContext *ctx, uint32_t insn, bool is_single, bool is_store,
    bool is_indexed, bool update)
{
    RTLUnit * const unit = ctx->unit;

    const RTLDataType type = is_single ? RTLTYPE_FLOAT32 : RTLTYPE_FLOAT64;
    const RTLOpcode rtlop = (ctx->handle->host_little_endian
                             ? (is_store ? RTLOP_STORE_BR : RTLOP_LOAD_BR)
                             : (is_store ? RTLOP_STORE : RTLOP_LOAD));

    int disp, ea;
    const int host_address =
        gen_load_store_address(ctx, insn, is_indexed, update, &disp, &ea);

    if (is_store) {
        /* If storing a single-precision value but the register is
         * currently in double-precision mode, we don't want to convert it
         * to single-precision because that would overwrite the register's
         * ps1 slot in the PSB. */
        int value;
        if (is_single && ctx->fpr_type[insn_frD(insn)] != RTLTYPE_FLOAT32
                      && ctx->fpr_type[insn_frD(insn)] != RTLTYPE_V2_FLOAT32) {
            const int f64 = get_fpr(ctx, insn_frD(insn), RTLTYPE_FLOAT64);
            value = rtl_alloc_register(unit, RTLTYPE_FLOAT32);
            rtl_add_insn(unit, RTLOP_FCAST, value, f64, 0, 0);
        } else {
            value = get_fpr(ctx, insn_frD(insn), type);
        }
        rtl_add_insn(unit, rtlop, 0, host_address, value, disp);
    } else {
        const int value = rtl_alloc_register(unit, type);
        rtl_add_insn(unit, rtlop, value, host_address, 0, disp);
        set_fpr(ctx, insn_frD(insn), value);
    }

    if (update) {
        if (is_indexed || insn_d(insn) != 0) {
            set_gpr(ctx, insn_rA(insn), ea);
        }
    }
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
static void translate_load_store_gpr(
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

    int disp, ea;
    const int host_address =
        gen_load_store_address(ctx, insn, is_indexed, update, &disp, &ea);

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
static void translate_load_store_multiple(
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
        flush_live_regs(ctx, false);
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
static void translate_load_store_string(
    GuestPPCContext *ctx, uint32_t insn, bool is_store, bool is_imm)
{
    RTLUnit * const unit = ctx->unit;

    /* We implement the string move instructions by loading or storing
     * directly to/from the PSB, so make sure it's up to date, and clear
     * the live register set so no future code tries to use stale values. */
    flush_live_regs(ctx, true);

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

        int real_offset;
        if (endian_flip) {
            real_offset = rtl_alloc_register(unit, RTLTYPE_ADDRESS);
            rtl_add_insn(unit, RTLOP_XORI,
                         real_offset, gpr_offset, 0, endian_flip);
        } else {
            real_offset = gpr_offset;
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
static void translate_logic_crb(
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
static void translate_logic_imm(
    GuestPPCContext *ctx, uint32_t insn, RTLOpcode rtlop, bool shift_imm,
    bool set_cr0)
{
    RTLUnit * const unit = ctx->unit;

    const int rS = get_gpr(ctx, insn_rS(insn));
    const uint32_t imm = shift_imm ? insn_UIMM(insn)<<16 : insn_UIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rS, 0, imm);
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
static void translate_logic_reg(
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
static void translate_move_spr(
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
            /* We have to flush rD since we set it differently depending on
             * whether a timebase handler function is present. */
            const int rD = insn_rD(insn);
            ctx->last_set.gpr[rD] = -1;
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
static void translate_muldiv_reg(
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
        ctx->last_set.gpr[rD] = -1;
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
            int noskip_label = rtl_alloc_label(unit);
            const int rA_is_80000000 = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI,
                         rA_is_80000000, rA, 0, UINT64_C(-0x80000000));
            rtl_add_insn(unit, RTLOP_GOTO_IF_Z,
                         0, rA_is_80000000, 0, noskip_label);
            const int rB_is_FFFFFFFF = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI, rB_is_FFFFFFFF, rB, 0, -1);
            rtl_add_insn(unit, RTLOP_GOTO_IF_NZ,
                         0, rB_is_FFFFFFFF, 0, div_skip_label);
            rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, noskip_label);
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
        rtl_add_insn(unit, RTLOP_ANDI, masked_xer, xer, 0, ~XER_OV);
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
            ctx->last_set.xer = -1;
            ctx->live.xer = 0;
            div_continue_label = rtl_alloc_label(unit);
            rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, div_continue_label);
        }

        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, div_skip_label);

        if (do_overflow) {
            const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ORI, new_xer, xer, 0, XER_SO | XER_OV);
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
            rtl_add_insn(unit, RTLOP_ANDI, rS_masked, rS_rotated, 0, ~mask);
            const int rA_masked = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ANDI, rA_masked, rA, 0, mask);
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
        rtl_add_insn(unit, RTLOP_ANDI, result, rotated, 0, mask);
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
            const uint32_t mask = (1u << insn_SH(insn)) - 1;
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

    flush_live_regs(ctx, false);
    guest_ppc_flush_cr(ctx, false);
    guest_ppc_flush_fpscr(ctx, false);
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
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     insn: Instruction word.
 */
static inline void translate_x1F(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn)
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
        int crb[4];
        for (int bit = 0; bit < 4; bit++) {
            crb[bit] = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_BFEXT,
                         crb[bit], xer, 0, (31-bit) | (1<<8));
        }
        for (int bit = 0; bit < 4; bit++) {
            set_crb(ctx, insn_crfD(insn)*4 + bit, crb[bit]);
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
            for (int i = 0; i < 32; i++) {
                if (ctx->last_set.crb[i] >= 0) {
                    rtl_opt_kill_insn(unit, ctx->last_set.crb[i], false);
                }
            }
            memset(ctx->last_set.crb, -1, sizeof(ctx->last_set.crb));
            memset(ctx->live.crb, 0, sizeof(ctx->live.crb));
            ctx->crb_loaded = 0;
            ctx->crb_dirty = 0;
        } else {
            const int rS = get_gpr(ctx, insn_rS(insn));
            for (int i = 0; i < 8; i++) {
                if (insn_CRM(insn) & (0x80 >> i)) {
                    int crb[4];
                    for (int j = 0; j < 4; j++) {
                        const int bit = i*4+j;
                        crb[j] = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_BFEXT,
                                     crb[j], rS, 0, (31-bit) | (1<<8));
                    }
                    for (int j = 0; j < 4; j++) {
                        const int bit = i*4+j;
                        set_crb(ctx, bit, crb[j]);
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
        set_crb(ctx, 2, zero);
        const int xer = get_xer(ctx);
        const int so = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFEXT, so, xer, 0, XER_SO_SHIFT | 1<<8);
        set_crb(ctx, 3, so);
        /* Flush CR0.eq because of the conditional branches. */
        ctx->last_set.crb[2] = -1;
        ctx->live.crb[2] = 0;
        ctx->crb_loaded |= 0x80000000u >> 2;
        ctx->crb_dirty |= 1u << 2;
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
        flush_live_regs(ctx, true);
        set_nia_imm(ctx, address + 4);
        post_insn_callback(ctx, address);
        rtl_add_insn(unit, RTLOP_GOTO,
                     0, 0, 0, guest_ppc_get_epilogue_label(ctx));
        return;
      case XO_DCBZ:
        translate_dcbz(ctx, insn);
        return;

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
        translate_load_store_fpr(ctx, insn, true, false, true, false);
        return;
      case XO_LFSUX:
        translate_load_store_fpr(ctx, insn, true, false, true, true);
        return;
      case XO_LFDX:
        translate_load_store_fpr(ctx, insn, false, false, true, false);
        return;
      case XO_LFDUX:
        translate_load_store_fpr(ctx, insn, false, false, true, true);
        return;
      case XO_STFSX:
        translate_load_store_fpr(ctx, insn, true, true, true, false);
        return;
      case XO_STFSUX:
        translate_load_store_fpr(ctx, insn, true, true, true, true);
        return;
      case XO_STFDX:
        translate_load_store_fpr(ctx, insn, false, true, true, false);
        return;
      case XO_STFDUX:
        translate_load_store_fpr(ctx, insn, false, true, true, true);
        return;
      case XO_STFIWX: {
        const RTLOpcode rtlop =
            ctx->handle->host_little_endian ? RTLOP_STORE_BR : RTLOP_STORE;
        const int host_address = get_ea_indexed(ctx, insn, NULL);
        const int f64 = get_fpr(ctx, insn_frD(insn), RTLTYPE_FLOAT64);
        const int i64 = rtl_alloc_register(unit, RTLTYPE_INT64);
        rtl_add_insn(unit, RTLOP_BITCAST, i64, f64, 0, 0);
        const int i32 = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_ZCAST, i32, i64, 0, 0);
        rtl_add_insn(unit, rtlop, 0, host_address, i32, 0);
        return;
      }  // case XO_STFIWX

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
        if (!insn_Rc(insn)
         && ((get_insn_at(ctx, block, address+4) & 0xFFE0FFFE)
             == (OPCD_RLWINM<<26 | insn_rA(insn)<<21 | 27<<11 | 5<<6 | 31<<1)))
        {
            /* "cntlzw temp,rX; srwi rY,temp,5" is a common PowerPC idiom
             * for comparing a value to zero and getting the result as an
             * integer rather than a condition flag.  If the temporary is
             * different from the output registers, we leave the cntlzw in
             * place in case its result happens to also be used elsewhere;
             * dead store elimination will remove it if not. */
            const uint32_t next_insn = get_insn_at(ctx, block, address+4);
            const int cntlzw_rS = insn_rS(insn);
            const int rlwinm_rA = insn_rA(next_insn);
            const int value = get_gpr(ctx, cntlzw_rS);
            /* Don't append the CLZ until after we retrieve the input
             * operand value, to correctly handle the case of cntlzw rN,rN
             * (overwriting the input operand). */
            if ((int)insn_rA(insn) != rlwinm_rA) {
                translate_bitmisc(ctx, insn, RTLOP_CLZ);
            }
            const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SEQI, result, value, 0, 0);
            set_gpr(ctx, rlwinm_rA, result);
            if (insn_Rc(next_insn)) {
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
            ctx->skip_next_insn = true;
            return;
        }
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
 * translate_x3F:  Translate the given opcode-0x3F instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn: Instruction word.
 */
static inline void translate_x3F(
    GuestPPCContext * const ctx, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (insn_XO_5(insn) & 0x10) {

        switch ((PPCExtendedOpcode3F_5)insn_XO_5(insn)) {
          default: return;  // FIXME: not yet implemented
        }

        ASSERT(!"Missing 0x3F_5 extended opcode handler");

    } else {  // !(insn_XO_5(insn) & 0x10)

        switch ((PPCExtendedOpcode3F_10)insn_XO_10(insn)) {
          case XO_FCMPU:
            translate_compare_fp(ctx, insn, false);
            return;

          case XO_FCMPO:
            translate_compare_fp(ctx, insn, true);
            return;

          case XO_MCRFS: {
            const int crfD_bit = insn_crfD(insn) * 4;
            int crb[4];

            if (insn_crfS(insn) == 4) {
                const int fprf = get_fr_fi_fprf(ctx);
                for (int i = 0; i < 4; i++) {
                    crb[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[i], fprf, 0, (3-i) | 1<<8);
                }
                for (int i = 0; i < 4; i++) {
                    set_crb(ctx, crfD_bit + i, crb[i]);
                }

            } else {  // crfS != 4
                const int crfS_bit = insn_crfS(insn) * 4;
                const int fpscr = get_fpscr(ctx);
                if (insn_crfS(insn) == 0) {
                    get_fpscr_fex_vx(ctx, fpscr, &crb[1], &crb[2]);
                    crb[0] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[0], fpscr, 0, 31 | 1<<8);
                    crb[3] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[3], fpscr, 0, 28 | 1<<8);
                } else if (insn_crfS(insn) == 3) {
                    const int fprf = get_fr_fi_fprf(ctx);
                    crb[0] = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 crb[0], fpscr, 0, 19 | 1<<8);
                    for (int i = 1; i < 4; i++) {
                        crb[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_BFEXT,
                                     crb[i], fprf, 0, (7-i) | 1<<8);
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        crb[i] = rtl_alloc_register(unit, RTLTYPE_INT32);
                        rtl_add_insn(unit, RTLOP_BFEXT, crb[i], fpscr, 0,
                                     (31-(crfS_bit+i)) | 1<<8);
                    }
                }
                for (int i = 0; i < 4; i++) {
                    set_crb(ctx, crfD_bit + i, crb[i]);
                }
                uint32_t mask = ((FPSCR_FX | FPSCR_ALL_EXCEPTIONS)
                                 & (0xF0000000u >> crfS_bit));
                if (mask) {
                    const int new_fpscr =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI, new_fpscr, fpscr, 0, ~mask);
                    set_fpscr(ctx, new_fpscr);
                }
            }

            return;
          }  // case XO_MCRFS

          case XO_MTFSB1:
          case XO_MTFSB0: {
            const uint32_t crbD_mask = 1u << (31 - insn_crbD(insn));

            if ((FPSCR_FR | FPSCR_FI | FPSCR_FPRF) & crbD_mask) {
                const int fprf = get_fr_fi_fprf(ctx);
                const int new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                const uint32_t mask = 1u << (19 - insn_crbD(insn));
                if (insn_XO_10(insn) == XO_MTFSB1) {
                    rtl_add_insn(unit, RTLOP_ORI, new_fprf, fprf, 0, mask);
                } else {
                    rtl_add_insn(unit, RTLOP_ANDI, new_fprf, fprf, 0, ~mask);
                }
                set_fr_fi_fprf(ctx, new_fprf);

            } else if ((FPSCR_FEX | FPSCR_VX | FPSCR_RESV20) & crbD_mask) {
                /* Do nothing -- these bits can't be written. */

            } else {
                const int fpscr = get_fpscr(ctx);
                const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                if (insn_XO_10(insn) == XO_MTFSB1) {
                    uint32_t mask = crbD_mask;
                    if (FPSCR_ALL_EXCEPTIONS & crbD_mask) {
                        mask |= FPSCR_FX;
                    }
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0, mask);
                } else {  // mtfsb0
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 new_fpscr, fpscr, 0, ~crbD_mask);
                }
                set_fpscr(ctx, new_fpscr);
                if (FPSCR_RN & crbD_mask) {
                    update_rounding_mode(ctx);
                }
            }

            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_MTFSB1, XO_MTFSB0

          case XO_MTFSFI: {
            const int crfD = insn_crfD(insn);

            if (crfD == 0) {
                const int fpscr = get_fpscr(ctx);
                const int masked_fpscr =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI,
                             masked_fpscr, fpscr, 0, 0x0FFFFFFF);
                int new_fpscr;
                if (insn_IMM(insn) & 9) {
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, masked_fpscr, 0,
                                 (insn_IMM(insn) & 9) << 28);
                } else {
                    new_fpscr = masked_fpscr;
                }
                set_fpscr(ctx, new_fpscr);

            } else if (crfD == 3) {
                const int fpscr = get_fpscr(ctx);
                const int new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                if (insn_IMM(insn) & 8) {
                    /* The omission of FPSCR_FX here is deliberate, since
                     * mtfsfi does not set FPSCR[FX] for nonzero crfD. */
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, fpscr, 0, 1u<<19);
                } else {
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 new_fpscr, fpscr, 0, ~(1u<<19));
                }
                set_fpscr(ctx, new_fpscr);
                const int fprf = get_fr_fi_fprf(ctx);
                const int masked_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, masked_fprf, fprf, 0, 0x0F);
                int new_fprf;
                if (insn_IMM(insn) & 7) {
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI, new_fprf, masked_fprf, 0,
                                 (insn_IMM(insn) & 7) << 4);
                } else {
                    new_fprf = masked_fprf;
                }
                set_fr_fi_fprf(ctx, new_fprf);

            } else if (crfD == 4) {
                const int fprf = get_fr_fi_fprf(ctx);
                const int masked_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_ANDI, masked_fprf, fprf, 0, 0x70);
                int new_fprf;
                if (insn_IMM(insn)) {
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI,
                                 new_fprf, masked_fprf, 0, insn_IMM(insn));
                } else {
                    new_fprf = masked_fprf;
                }
                set_fr_fi_fprf(ctx, new_fprf);

            } else {  // crfD not in {0,3,4}
                const int fpscr = get_fpscr(ctx);
                const int masked_fpscr =
                    rtl_alloc_register(unit, RTLTYPE_INT32);
                uint32_t mask = 0xF0000000u >> (insn_crfD(insn) * 4);
                rtl_add_insn(unit, RTLOP_ANDI, masked_fpscr, fpscr, 0, ~mask);
                int imm = insn_IMM(insn);
                if (insn_crfD(insn) == 5) {
                    imm &= 7;
                }
                int new_fpscr;
                if (insn_IMM(insn)) {
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ORI, new_fpscr, masked_fpscr, 0,
                                 imm << (28 - insn_crfD(insn)*4));
                } else {
                    new_fpscr = masked_fpscr;
                }
                set_fpscr(ctx, new_fpscr);
                if (insn_crfD(insn) == 7) {
                    static const uint8_t rounding_mode[4] = {
                        [FPSCR_RN_N] = RTLFROUND_NEAREST,
                        [FPSCR_RN_Z] = RTLFROUND_TRUNC,
                        [FPSCR_RN_P] = RTLFROUND_CEIL,
                        [FPSCR_RN_M] = RTLFROUND_FLOOR,
                    };
                    rtl_add_insn(unit, RTLOP_FSETROUND,
                                 0, 0, 0, rounding_mode[imm & 3]);
                }
            }  // if (crfD == ...)

            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_MTFSFI

          case XO_MFFS: {
            int fex, vx;
            get_fpscr_fex_vx(ctx, get_fpscr(ctx), &fex, &vx);
            const int fpscr = merge_fpscr(ctx, true);
            const int shifted_fex = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI,
                         shifted_fex, fex, 0, FPSCR_FEX_SHIFT);
            const int shifted_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_SLLI, shifted_vx, vx, 0, FPSCR_VX_SHIFT);
            const int fex_vx = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, fex_vx, shifted_fex, shifted_vx, 0);
            const int final_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_OR, final_fpscr, fpscr, fex_vx, 0);
            const int fpscr64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_ZCAST, fpscr64, final_fpscr, 0, 0);
            const int result = rtl_alloc_register(unit, RTLTYPE_FLOAT64);
            rtl_add_insn(unit, RTLOP_BITCAST, result, fpscr64, 0, 0);
            set_fpr(ctx, insn_frD(insn), result);
            if (insn_Rc(insn)) {
                /* We already have FEX/VX, so avoid recomputing them. */
                const int fx = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT, fx, fpscr, 0, 31 | 1<<8);
                const int ox = rtl_alloc_register(unit, RTLTYPE_INT32);
                rtl_add_insn(unit, RTLOP_BFEXT, ox, fpscr, 0, 28 | 1<<8);
                set_crb(ctx, 4, fx);
                set_crb(ctx, 5, fex);
                set_crb(ctx, 6, vx);
                set_crb(ctx, 7, ox);
            }
            return;
          }  // case XO_MFFS

          case XO_MTFSF: {
            const int frB = get_fpr(ctx, insn_frB(insn), RTLTYPE_FLOAT64);
            const int bits64 = rtl_alloc_register(unit, RTLTYPE_INT64);
            rtl_add_insn(unit, RTLOP_BITCAST, bits64, frB, 0, 0);
            const int bits = rtl_alloc_register(unit, RTLTYPE_INT32);
            rtl_add_insn(unit, RTLOP_ZCAST, bits, bits64, 0, 0);

            const int FM_fpscr = insn_FM(insn) & 0xF7;
            const int FM_fprf = insn_FM(insn) & 0x18;

            if (FM_fpscr) {
                int new_fpscr;
                if (FM_fpscr == 0xF7) {
                    const uint32_t mask = ~(
                        FPSCR_FEX | FPSCR_VX | FPSCR_FR | FPSCR_FI
                        | FPSCR_FPRF | FPSCR_RESV20);
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI, new_fpscr, bits, 0, mask);
                } else {
                    uint32_t mask = 0;
                    for (int i = 0; i < 8; i++) {
                        if (FM_fpscr & (1 << i)) {
                            mask |= 0xF << (i*4);
                        }
                    }
                    mask &= ~(FPSCR_FEX | FPSCR_VX | FPSCR_FR | FPSCR_FI
                              | FPSCR_FPRF | FPSCR_RESV20);
                    const int fpscr = get_fpscr(ctx);
                    const int masked_bits =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI, masked_bits, bits, 0, mask);
                    const int masked_fpscr =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_ANDI,
                                 masked_fpscr, fpscr, 0, ~mask);
                    new_fpscr = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_OR,
                                 new_fpscr, masked_fpscr, masked_bits, 0);
                }
                set_fpscr(ctx, new_fpscr);
                if (FM_fpscr & 0x01) {
                    update_rounding_mode(ctx);
                }
            }

            if (FM_fprf) {
                int new_fprf;
                if (FM_fprf == 0x18) {
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_BFEXT,
                                 new_fprf, bits, 0, FPSCR_FPRF_SHIFT | 7<<8);
                } else {
                    const int fprf = get_fr_fi_fprf(ctx);
                    const int shifted_bits =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_SRLI,
                                 shifted_bits, bits, 0, FPSCR_FPRF_SHIFT);
                    const int masked_bits =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    const int masked_fprf =
                        rtl_alloc_register(unit, RTLTYPE_INT32);
                    if (FM_fprf == 0x10) {
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_bits, shifted_bits, 0, 0x70);
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_fprf, fprf, 0, 0x0F);
                    } else {
                        ASSERT(FM_fprf == 0x08);
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_bits, shifted_bits, 0, 0x0F);
                        rtl_add_insn(unit, RTLOP_ANDI,
                                     masked_fprf, fprf, 0, 0x70);
                    }
                    new_fprf = rtl_alloc_register(unit, RTLTYPE_INT32);
                    rtl_add_insn(unit, RTLOP_OR,
                                 new_fprf, masked_fprf, masked_bits, 0);
                }
                set_fr_fi_fprf(ctx, new_fprf);
            }

            if (insn_Rc(insn)) {
                update_cr1(ctx);
            }
            return;
          }  // case XO_MTFSF

          default: return;  // FIXME: not yet implemented
        }

        ASSERT(!"Missing 0x3F_10 extended opcode handler");

    }  // if (insn_XO_5(insn) & 0x10)
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

      case OPCD_x04:
        switch ((PPCExtendedOpcode04_750CL_5)insn_XO_5(insn)) {
          case XO_PS_MISC:
            ASSERT(insn_XO_10(insn) == XO_DCBZ_L);
            /* We treat "locked" cache identically to normal cache. */
            translate_dcbz(ctx, insn);
            return;
          default: return;  // FIXME: not yet implemented
        }
        ASSERT(!"Missing 0x04 extended opcode handler");

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
        guest_ppc_flush_fpscr(ctx, true);
        flush_live_regs(ctx, true);
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
          case XO_MCRF: {
            int crb[4];
            for (int i = 0; i < 4; i++) {
                crb[i] = get_crb(ctx, insn_crfS(insn)*4 + i);
            }
            for (int i = 0; i < 4; i++) {
                set_crb(ctx, insn_crfD(insn)*4 + i, crb[i]);
            }
            return;
          }
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
        translate_x1F(ctx, block, address, insn);
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

      case OPCD_LFS:
        translate_load_store_fpr(ctx, insn, true, false, false, false);
        return;

      case OPCD_LFSU:
        translate_load_store_fpr(ctx, insn, true, false, false, true);
        return;

      case OPCD_LFD:
        translate_load_store_fpr(ctx, insn, false, false, false, false);
        return;

      case OPCD_LFDU:
        translate_load_store_fpr(ctx, insn, false, false, false, true);
        return;

      case OPCD_STFS:
        translate_load_store_fpr(ctx, insn, true, true, false, false);
        return;

      case OPCD_STFSU:
        translate_load_store_fpr(ctx, insn, true, true, false, true);
        return;

      case OPCD_STFD:
        translate_load_store_fpr(ctx, insn, false, true, false, false);
        return;

      case OPCD_STFDU:
        translate_load_store_fpr(ctx, insn, false, true, false, true);
        return;

      case OPCD_PSQ_L:
      case OPCD_PSQ_LU:
      case OPCD_x3B:
      case OPCD_PSQ_ST:
      case OPCD_PSQ_STU:
        return;  // FIXME: not yet implemented

      case OPCD_x3F:
        translate_x3F(ctx, insn);
        return;

    }  // switch (insn_OPCD(insn))

    ASSERT(!"Missing primary opcode handler");
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

bool guest_ppc_translate_block(GuestPPCContext *ctx, int index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(index >= 0 && index < ctx->num_blocks);

    ctx->current_block = index;

    RTLUnit * const unit = ctx->unit;
    GuestPPCBlockInfo *block = &ctx->blocks[index];
    const uint32_t start = block->start;
    const uint32_t *memory_base =
        (const uint32_t *)ctx->handle->setup.guest_memory_base;

    if (block->is_branch_target) {
        rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, block->label);
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to add label at 0x%X", start);
            return false;
        }
    }

    if (UNLIKELY(block->len == 0)) {
        /* This block was a backward branch target that wasn't part of a
         * previous block (see block-splitting logic at the bottom of
         * guest_ppc_scan()).  Update NIA and return to the caller to
         * retranslate from the target address. */
        set_nia_imm(ctx, start);
        rtl_add_insn(unit, RTLOP_GOTO,
                     0, 0, 0, guest_ppc_get_epilogue_label(ctx));
        if (UNLIKELY(rtl_get_error_state(unit))) {
            log_ice(ctx->handle, "Failed to translate empty block at 0x%X",
                    start);
            return false;
        }
        return true;
    }

    memset(&ctx->live, 0, sizeof(ctx->live));
    memset(ctx->fpr_type, 0, sizeof(ctx->fpr_type));
    ctx->crb_dirty = 0;
    memset(&ctx->last_set, -1, sizeof(ctx->last_set));

    ctx->skip_next_insn = false;

    for (uint32_t ofs = 0; ofs < block->len; ofs += 4) {
        const uint32_t address = start + ofs;
        if (ctx->handle->pre_insn_callback) {
            flush_live_regs(ctx, false);
            guest_ppc_flush_cr(ctx, false);
            guest_ppc_flush_fpscr(ctx, false);
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

    flush_live_regs(ctx, true);
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

    if (ctx->crb_loaded) {
        const int cr = merge_cr(ctx, make_live);
        if (make_live) {
            set_cr(ctx, cr);
            memset(ctx->last_set.crb, -1, sizeof(ctx->last_set.crb));
            memset(ctx->live.crb, 0, sizeof(ctx->live.crb));
            ctx->crb_loaded = 0;
            ctx->crb_dirty = 0;
        } else {
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, cr, 0, ctx->alias.cr);
        }
    }
}

/*-----------------------------------------------------------------------*/

void guest_ppc_flush_fpscr(GuestPPCContext *ctx, bool make_live)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    RTLUnit * const unit = ctx->unit;

    if (ctx->alias.fpscr) {
        const int fpscr = merge_fpscr(ctx, make_live);
        if (make_live) {
            set_fpscr(ctx, fpscr);
            ctx->last_set.fr_fi_fprf = -1;
            ctx->live.fr_fi_fprf = 0;
        } else {
            rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, fpscr, 0, ctx->alias.fpscr);
        }
    }
}

/*-----------------------------------------------------------------------*/

int guest_ppc_get_epilogue_label(GuestPPCContext *ctx)
{
    if (!ctx->epilogue_label) {
        ctx->epilogue_label = rtl_alloc_label(ctx->unit);
    }
    return ctx->epilogue_label;
}

/*************************************************************************/
/*************************************************************************/
