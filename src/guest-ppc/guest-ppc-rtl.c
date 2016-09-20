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

/* Report the current address in error messages. */
#undef ICE_SUFFIX
#define ICE_SUFFIX  " at 0x%X", address

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
 * [Return value]
 *     True on success, false on error.
 */
static bool update_cr0(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const int result, int xer)
{
    RTLUnit * const unit = ctx->unit;

    if (!xer) {
        ALLOC_REGISTER(xer, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
    }

    DECLARE_NEW_REGISTER(lt, RTLTYPE_INT32);
    ADD_INSN(RTLOP_SLTSI, lt, result, 0, 0);
    DECLARE_NEW_REGISTER(gt, RTLTYPE_INT32);
    ADD_INSN(RTLOP_SGTSI, gt, result, 0, 0);
    DECLARE_NEW_REGISTER(eq, RTLTYPE_INT32);
    ADD_INSN(RTLOP_SEQI, eq, result, 0, 0);
    DECLARE_NEW_REGISTER(so, RTLTYPE_INT32);
    ADD_INSN(RTLOP_BFEXT, so, xer, 0, 31 | 1<<8);

    DECLARE_NEW_REGISTER(cr0_2, RTLTYPE_INT32);
    ADD_INSN(RTLOP_BFINS, cr0_2, so, eq, 1 | 1<<8);
    DECLARE_NEW_REGISTER(cr0_3, RTLTYPE_INT32);
    ADD_INSN(RTLOP_BFINS, cr0_3, cr0_2, gt, 2 | 1<<8);
    DECLARE_NEW_REGISTER(cr0, RTLTYPE_INT32);
    ADD_INSN(RTLOP_BFINS, cr0, cr0_3, lt, 3 | 1<<8);
    ADD_INSN(RTLOP_SET_ALIAS, 0, cr0, 0, ctx->alias.cr[0]);

    return true;
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
 * [Return value]
 *     True on success, false on error.
 */
static bool translate_arith_imm(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn, const RTLOpcode rtlop,
    const bool shift_imm, const bool set_ca, const bool set_cr0)
{
    RTLUnit * const unit = ctx->unit;

    DECLARE_NEW_REGISTER(rA, RTLTYPE_INT32);
    ADD_INSN(RTLOP_GET_ALIAS, rA, 0, 0, ctx->alias.gpr[get_rA(insn)]);
    const int32_t imm = shift_imm ? get_SIMM(insn)<<16 : get_SIMM(insn);
    DECLARE_NEW_REGISTER(result, RTLTYPE_INT32);
    ADD_INSN(rtlop, result, rA, 0, imm);
    ADD_INSN(RTLOP_SET_ALIAS, 0, result, 0, ctx->alias.gpr[get_rD(insn)]);
    ctx->live.gpr[get_rD(insn)] = result;

    int xer = 0;
    if (set_ca) {
        ASSERT(rtlop == RTLOP_ADDI);
        ALLOC_REGISTER(xer, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
        DECLARE_NEW_REGISTER(ca, RTLTYPE_INT32);
        ADD_INSN(RTLOP_SLTUI, ca, result, 0, imm);
        DECLARE_NEW_REGISTER(new_xer, RTLTYPE_INT32);
        ADD_INSN(RTLOP_BFINS, new_xer, xer, ca, 29 | 1<<8);
        ADD_INSN(RTLOP_SET_ALIAS, 0, new_xer, 0, ctx->alias.xer);
    }

    if (set_cr0) {
        return update_cr0(ctx, block, address, result, xer);
    } else {
        return true;
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
static inline bool translate_insn(
    GuestPPCContext * const ctx, GuestPPCBlockInfo * const block,
    const uint32_t address, const uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (UNLIKELY(!is_valid_insn(insn))) {
        ADD_INSN(RTLOP_ILLEGAL, 0, 0, 0, 0);
        return true;
    }

    switch (get_OPCD(insn)) {
      case OPCD_MULLI:
        return translate_arith_imm(ctx, block, address, insn,
                                   RTLOP_MULI, false, false, false);

      case OPCD_SUBFIC: {
        DECLARE_NEW_REGISTER(rA, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, rA, 0, 0, ctx->alias.gpr[get_rA(insn)]);
        DECLARE_NEW_REGISTER(imm, RTLTYPE_INT32);
        ADD_INSN(RTLOP_LOAD_IMM, imm, 0, 0, (uint32_t)get_SIMM(insn));
        DECLARE_NEW_REGISTER(result, RTLTYPE_INT32);
        ADD_INSN(RTLOP_SUB, result, imm, rA, 0);
        ADD_INSN(RTLOP_SET_ALIAS, 0, result, 0, ctx->alias.gpr[get_rD(insn)]);
        ctx->live.gpr[get_rD(insn)] = result;

        DECLARE_NEW_REGISTER(xer, RTLTYPE_INT32);
        ADD_INSN(RTLOP_GET_ALIAS, xer, 0, 0, ctx->alias.xer);
        DECLARE_NEW_REGISTER(ca, RTLTYPE_INT32);
        ADD_INSN(RTLOP_SGTU, ca, imm, result, 0);
        DECLARE_NEW_REGISTER(new_xer, RTLTYPE_INT32);
        ADD_INSN(RTLOP_BFINS, new_xer, xer, ca, 29 | 1<<8);
        ADD_INSN(RTLOP_SET_ALIAS, 0, new_xer, 0, ctx->alias.xer);

        return true;
      }

      case OPCD_ADDIC:
        return translate_arith_imm(ctx, block, address, insn,
                                   RTLOP_ADDI, false, true, false);

      case OPCD_ADDIC_:
        return translate_arith_imm(ctx, block, address, insn,
                                   RTLOP_ADDI, false, true, true);

      case OPCD_ADDI:
        if (get_rA(insn) == 0) {
            DECLARE_NEW_REGISTER(result, RTLTYPE_INT32);
            ADD_INSN(RTLOP_LOAD_IMM, result, 0, 0, (uint32_t)get_SIMM(insn));
            ADD_INSN(RTLOP_SET_ALIAS,
                     0, result, 0, ctx->alias.gpr[get_rD(insn)]);
            ctx->live.gpr[get_rD(insn)] = result;
            return true;
        } else {
            return translate_arith_imm(ctx, block, address, insn,
                                       RTLOP_ADDI, false, false, false);
        }

      case OPCD_ADDIS:
        return translate_arith_imm(ctx, block, address, insn,
                                   RTLOP_ADDI, true, false, false);

      default: return false;  // FIXME: not yet implemented
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

    #undef ICE_SUFFIX
    #define ICE_SUFFIX  " at 0x%X", start
    ADD_INSN(RTLOP_LABEL, 0, 0, 0, block->label);

    memset(&ctx->live, 0, sizeof(ctx->live));
    for (uint32_t ofs = 0; ofs < block->len; ofs += 4) {
        const uint32_t address = start + ofs;
        const uint32_t insn = bswap_be32(memory_base[address/4]);
        if (UNLIKELY(!translate_insn(ctx, block, address, insn))) {
            return false;
        }
    }

    #undef ICE_SUFFIX
    #define ICE_SUFFIX  " at 0x%X", start + block->len
    DECLARE_NEW_REGISTER(nia_reg, RTLTYPE_INT32);
    ADD_INSN(RTLOP_LOAD_IMM, nia_reg, 0, 0, start + block->len);
    ADD_INSN(RTLOP_SET_ALIAS, 0, nia_reg, 0, ctx->alias.nia);

    return true;
}

/*************************************************************************/
/*************************************************************************/
