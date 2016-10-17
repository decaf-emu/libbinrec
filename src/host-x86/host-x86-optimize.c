/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/host-x86.h"
#include "src/host-x86/host-x86-internal.h"
#include "src/rtl-internal.h"

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * is_reg_killable:  Return whether the given register has no references
 * other than the instruction that sets it and the given instruction
 * (implying that the register can be eliminated with no effect on other
 * code).
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

    if (reg->death != insn_index) {
        return false;
    }
    for (int i = insn_index - 1; i > reg->birth; i--) {
        const RTLInsn * const check_insn = &unit->insns[i];
        if (check_insn->src1 == reg_index || check_insn->src2 == reg_index) {
            return false;
        }
        const bool has_src3 = (check_insn->opcode == RTLOP_SELECT
                            || check_insn->opcode == RTLOP_CMPXCHG
                            || check_insn->opcode == RTLOP_CALL
                            || check_insn->opcode == RTLOP_CALL_TRANSPARENT);
        if (has_src3 && check_insn->src3 == reg_index) {
            return false;
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * kill_reg:  Eliminate the given register and the instruction that sets it.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg_index: Register to eliminate.
 */
static void kill_reg(const RTLUnit * const unit, const int reg_index)
{
    RTLRegister * const reg = &unit->regs[reg_index];
    RTLInsn * const birth_insn = &unit->insns[reg->birth];

    birth_insn->opcode = RTLOP_NOP;
    birth_insn->dest = 0;
    birth_insn->src1 = 0;
    birth_insn->src2 = 0;
    birth_insn->src_imm = 0;

    reg->live = false;
    reg->death = reg->birth;
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
    const RTLUnit * const unit, const int reg_index, const int insn_index)
{
    RTLRegister * const reg = &unit->regs[reg_index];

    if ((reg->source == RTLREG_RESULT || reg->source == RTLREG_RESULT_NOFOLD)
     && reg->result.opcode == RTLOP_ZCAST
     && is_reg_killable(unit, reg_index, insn_index)) {
        const int zcast_src = reg->result.src1;
        kill_reg(unit, reg_index);
        return zcast_src;
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
    kill_reg(unit, src1);

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
        unit->regs[addend1].death = insn_index;
    }
    if (addend2 && unit->regs[addend2].death < insn_index) {
        unit->regs[addend2].death = insn_index;
    }
}

/*************************************************************************/
/*************************************************************************/
