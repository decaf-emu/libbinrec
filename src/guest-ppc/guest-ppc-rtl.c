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
    GuestPPCContext *ctx, GuestPPCBlockInfo *block,
    uint32_t address, uint32_t insn)
{
    RTLUnit * const unit = ctx->unit;

    if (UNLIKELY(!is_valid_insn(insn))) {
        ADD_INSN(RTLOP_ILLEGAL, 0, 0, 0, 0);
        return true;
    }

    switch (get_OPCD(insn)) {
      case OPCD_ADDI: {
        DECLARE_NEW_REGISTER(result, RTLTYPE_INT32);
        const uint32_t imm = get_SIMM(insn);
        if (get_rA(insn) != 0) {
            // FIXME: not yet implemented
        } else {
            ADD_INSN(RTLOP_LOAD_IMM, result, 0, 0, imm);
        }
        ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.gpr[get_rD(insn)], result, 0, 0);
        ctx->live.gpr[get_rD(insn)] = result;
        break;
      }

      default:
        break;  // FIXME: not yet implemented
    }  // switch (get_OPCD(insn))

    return true;
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
    ADD_INSN(RTLOP_SET_ALIAS, ctx->alias.nia, nia_reg, 0, 0);

    return true;
}

/*************************************************************************/
/*************************************************************************/
