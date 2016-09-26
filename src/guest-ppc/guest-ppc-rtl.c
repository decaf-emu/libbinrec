/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/endian.h"
#include "src/guest-ppc/guest-ppc-decode.h"
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl.h"

/*************************************************************************/
/****************************** Local data *******************************/
/*************************************************************************/

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * update_cr0:  Add RTL instructions to update the value of CR0 based on
 * the result of an integer operation.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block: Basic block being translated.
 *     address: Address of instruction being translated.
 *     result: RTL register containing operation result.
 *     xer: RTL register containing current value of XER, or 0 if none.
 */
static void update_cr0(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const int result, int xer)
{
    RTLUnit * const unit = ctx->unit;

    if (!xer) {
        xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
    }

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
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, cr0, 0, ctx->alias.cr[0]);
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

    const int rA = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_GET_ALIAS,
                 rA, 0, 0, ctx->alias.gpr[get_rA(insn)]);
    const int32_t imm = shift_imm ? get_SIMM(insn)<<16 : get_SIMM(insn);
    const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, rtlop, result, rA, 0, imm);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0,
                 result, 0, ctx->alias.gpr[get_rD(insn)]);
    ctx->live.gpr[get_rD(insn)] = result;

    int xer = 0;
    if (set_ca) {
        ASSERT(rtlop == RTLOP_ADDI);
        xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SLTUI, ca, result, 0, imm);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, 29 | 1<<8);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_xer, 0, ctx->alias.xer);
    }

    if (set_cr0) {
        update_cr0(ctx, block, address, result, xer);
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
        const int rA = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS,
                     rA, 0, 0, ctx->alias.gpr[get_rA(insn)]);
        const int imm = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_LOAD_IMM,
                     imm, 0, 0, (uint32_t)get_SIMM(insn));
        const int result = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SUB, result, imm, rA, 0);
        rtl_add_insn(unit, RTLOP_SET_ALIAS,
                     0, result, 0, ctx->alias.gpr[get_rD(insn)]);
        ctx->live.gpr[get_rD(insn)] = result;

        const int xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
        const int ca = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_SGTU, ca, imm, result, 0);
        const int new_xer = rtl_alloc_register(unit, RTLTYPE_INT32);
        rtl_add_insn(unit, RTLOP_BFINS, new_xer, xer, ca, 29 | 1<<8);
        rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, new_xer, 0, ctx->alias.xer);

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
            rtl_add_insn(unit, RTLOP_SET_ALIAS,
                         0, result, 0, ctx->alias.gpr[get_rD(insn)]);
            ctx->live.gpr[get_rD(insn)] = result;
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

/*************************************************************************/
/************************ Translation entry point ************************/
/*************************************************************************/

bool guest_ppc_gen_rtl(GuestPPCContext *ctx, int index)
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

    const int nia_reg = rtl_alloc_register(unit, RTLTYPE_INT32);
    rtl_add_insn(unit, RTLOP_LOAD_IMM, nia_reg, 0, 0, start + block->len);
    rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, nia_reg, 0, ctx->alias.nia);
    if (UNLIKELY(rtl_get_error_state(unit))) {
        log_ice(ctx->handle, "Failed to store NIA at 0x%X",
                start + block->len);
        return false;
    }

    return true;
}

/*************************************************************************/
/*************************************************************************/
