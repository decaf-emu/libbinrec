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
#include "src/host-x86.h"
#include "src/host-x86/host-x86-internal.h"
#include "src/rtl-internal.h"

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * is_reg_killable:  Return whether the given register has no references
 * between the instruction that sets it and the given instruction (implying
 * that the register can be eliminated with no effect on other code if the
 * register dies at the given instruction).
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg_index: Register to check.
 *     insn_index: Instruction containing register reference.
 * [Return value]
 *     True if no instructions other than insn_index read from the given
 *     register; false otherwise.
 */
static PURE_FUNCTION bool is_reg_killable(
    const RTLUnit * const unit, const int reg_index, const int insn_index)
{
    const RTLRegister * const reg = &unit->regs[reg_index];
    if (reg->death > insn_index) {
        return false;
    } else {
        return rtl_opt_prev_reg_use(unit, reg_index, insn_index) == reg->birth;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * kill_reg:  Eliminate the given register and the instruction that sets it.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg_index: Register to eliminate.
 *     dse: True to recursively eliminate dead registers.
 */
static void kill_reg(RTLUnit * const unit, const int reg_index, bool dse)
{
    RTLRegister * const reg = &unit->regs[reg_index];
    reg->death = reg->birth;
    rtl_opt_kill_insn(unit, reg->birth, dse);
}

/*-----------------------------------------------------------------------*/

/**
 * maybe_eliminate_zcast:  If the given register is the result of a ZCAST
 * instruction and is not referenced by any instructions except that ZCAST
 * and the instruction given by insn_index, eliminate that register and the
 * ZCAST that sets it.  Helper function for host_x86_optimize_address().
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg_index: Register to check.
 *     insn_index: Instruction containing register reference.
 * [Return value]
 *     New register index (which is the same as the original register if
 *     it was not eliminated).
 */
static int maybe_eliminate_zcast(
    RTLUnit * const unit, const int reg_index, const int insn_index)
{
    RTLRegister * const reg = &unit->regs[reg_index];

    if ((reg->source == RTLREG_RESULT || reg->source == RTLREG_RESULT_NOFOLD)
     && reg->result.opcode == RTLOP_ZCAST) {
        const int zcast_src = reg->result.src1;
        if (is_reg_killable(unit, reg_index, insn_index)) {
            /* Don't kill recursively because we want to keep the source
             * operand. */
            kill_reg(unit, reg_index, false);
            return zcast_src;
        }
    }

    return reg_index;
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

void host_x86_optimize_address(HostX86Context * const ctx, int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);
    ASSERT(ctx->unit->insns[insn_index].opcode >= RTLOP_LOAD
           && ctx->unit->insns[insn_index].opcode <= RTLOP_CMPXCHG);

    RTLUnit * const unit = ctx->unit;
    RTLInsn * const insn = &unit->insns[insn_index];
    const int src1 = insn->src1;
    RTLRegister * const src1_reg = &unit->regs[src1];

    /* Check whether the address is the sum of two values, and if so,
     * extract those values from the instruction.  We allow RESULT_NOFOLD
     * as well as RESULT since if constant folding is enabled, any RESULT
     * register which couldn't be folded will have been changed to
     * RESULT_NOFOLD. */
    int32_t addend1, addend2, offset;
    if (src1_reg->source != RTLREG_RESULT
     && src1_reg->source != RTLREG_RESULT_NOFOLD) {
        return;  // Can't optimize.
    }
    if (src1_reg->result.opcode == RTLOP_ADD) {
        addend1 = src1_reg->result.src1;
        addend2 = src1_reg->result.src2;
        offset = 0;
    } else if (src1_reg->result.opcode == RTLOP_ADDI) {
        addend1 = src1_reg->result.src1;
        addend2 = 0;
        offset = src1_reg->result.src_imm;
    } else {
        return;  // Can't optimize.
    }

    /* Bail if the instruction is ADDI and the final offset would be
     * outside the range of a 32-bit displacement. */
    const bool insn_has_offset = (insn->opcode < RTLOP_ATOMIC_INC);
    if (insn_has_offset) {
        const int64_t final_offset = (int64_t)offset + (int64_t)insn->offset;
        if ((uint64_t)(final_offset+0x80000000u) >= UINT64_C(0x100000000)) {
            return;
        }
        offset = (int32_t)final_offset;
    }

    /* If the register is referenced by any other instruction, we need to
     * save the result of the addition in its own register anyway, so
     * there's no benefit to optimization.  (If the other instructions are
     * also memory-access instructions, in theory we could optimize them
     * all and eliminate the addition instruction, but guest translators
     * will typically generate one-off ADDs for memory accesses, so we
     * don't worry aobut that case.) */
    if (!is_reg_killable(unit, src1, insn_index)) {
        return;
    }

    /* At this point, the optimization is guaranteed to succeed. */

    /* Eliminate the addition instruction and its output. */
    const int add_insn_index = src1_reg->birth;
    kill_reg(unit, src1, false);

    /* If either operand is the output of a ZCAST instruction (and is not
     * used elsewhere), replace it with the ZCAST input and eliminate the
     * ZCAST.  This is safe even for RTLTYPE_INT32 registers because the
     * x86 architecture specifies that all 32-bit operations clear the
     * high 32 bits of the output register in 64-bit mode. */
    addend1 = maybe_eliminate_zcast(unit, addend1, add_insn_index);
    if (addend2) {
        addend2 = maybe_eliminate_zcast(unit, addend2, add_insn_index);
    }

    /* Update the memory access instruction with the new operands. */
    insn->src1 = addend1;
    insn->host_data_16 = addend2;
    insn->host_data_32 = offset;
    /* For instructions with offsets (pure load/store as opposed to atomic
     * RMW), also clear the offset field in case it was nonzero but the sum
     * of the offset and the immediate operand is zero. */
    if (insn_has_offset) {
        insn->offset = 0;
    }

    /* Extend the live ranges of the operands in case they died before
     * the memory access instruction. */
    if (unit->regs[addend1].death < insn_index) {
#ifdef RTL_DEBUG_OPTIMIZE
        log_info(unit->handle, "Extending r%d live range to %d",
                 addend1, insn_index);
#endif
        unit->regs[addend1].death = insn_index;
    }
    if (addend2 && unit->regs[addend2].death < insn_index) {
#ifdef RTL_DEBUG_OPTIMIZE
        log_info(unit->handle, "Extending r%d live range to %d",
                 addend2, insn_index);
#endif
        unit->regs[addend2].death = insn_index;
    }
}

/*-----------------------------------------------------------------------*/

void host_x86_optimize_immediate_store(HostX86Context * const ctx,
                                       int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);
    ASSERT((ctx->unit->insns[insn_index].opcode >= RTLOP_STORE
         && ctx->unit->insns[insn_index].opcode <= RTLOP_STORE_I16)
        || (ctx->unit->insns[insn_index].opcode >= RTLOP_STORE_BR
         && ctx->unit->insns[insn_index].opcode <= RTLOP_STORE_I16_BR));
    ASSERT(ctx->unit->regs[ctx->unit->insns[insn_index].src2].source
           == RTLREG_CONSTANT);


    RTLUnit * const unit = ctx->unit;
    RTLInsn * const insn = &unit->insns[insn_index];
    const int src2 = insn->src2;
    RTLRegister * const src2_reg = &unit->regs[src2];

    /* Don't optimize out the constant if it's also the store address.
     * Which should be extremely rare, but who knows... */
    if (UNLIKELY(src2 == insn->src1)) {
        return;
    }

    if (!is_reg_killable(unit, src2, insn_index)) {
        return;
    }

    RTLOpcode opcode = insn->opcode;
    RTLDataType type = src2_reg->type;
    if (type == RTLTYPE_FLOAT32) {
        type = RTLTYPE_INT32;
    } else if (type == RTLTYPE_FLOAT64) {
        type = RTLTYPE_INT64;
    }
    uint64_t value = src2_reg->value.i64;
    if (opcode == RTLOP_STORE_BR) {
        opcode = RTLOP_STORE;
        if (src2_reg->type == RTLTYPE_INT32) {
            value = bswap32((uint32_t)value);
        } else {
            value = bswap64(value);
        }
    } else if (opcode == RTLOP_STORE_I16_BR) {
        opcode = RTLOP_STORE_I16;
        value = bswap16((uint16_t)value);
    }
    if (type != RTLTYPE_INT32 && value+0x80000000u >= UINT64_C(0x100000000)) {
        return;  // The constant won't fit in a single instruction.
    }

    kill_reg(unit, src2, false);
    ASSERT(!src2_reg->live);
    src2_reg->type = type;
    src2_reg->value.i64 = value;
    insn->opcode = opcode;
}

/*************************************************************************/
/*************************************************************************/
