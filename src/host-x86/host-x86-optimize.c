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

    if (src1_reg->source != RTLREG_RESULT
     && src1_reg->source != RTLREG_RESULT_NOFOLD) {
        return;  // Can't optimize.
    }

    if (src1_reg->death != insn_index) {
        return;  // No benefit to optimization.
    }
    for (int i = insn_index - 1; i > src1_reg->birth; i--) {
        const RTLInsn * const check_insn = &unit->insns[i];
        if (check_insn->src1 == src1 || check_insn->src2 == src1) {
            return;  // Still used, so no benefit to optimization.
        }
        const bool has_src3 = (check_insn->opcode == RTLOP_SELECT
                            || check_insn->opcode == RTLOP_CMPXCHG
                            || check_insn->opcode == RTLOP_CALL
                            || check_insn->opcode == RTLOP_CALL_TRANSPARENT);
        if (has_src3 && check_insn->src3 == src1) {
            return;  // No benefit to optimization.
        }
    }

    int32_t addend1, addend2;
    bool addend2_is_imm;
    if (src1_reg->result.opcode == RTLOP_ADD) {
        addend1 = src1_reg->result.src1;
        addend2 = src1_reg->result.src2;
        addend2_is_imm = false;
    } else if (src1_reg->result.opcode == RTLOP_ADDI) {
        addend1 = src1_reg->result.src1;
        addend2 = src1_reg->result.src_imm;
        addend2_is_imm = true;
    } else {
        return;  // Can't optimize.
    }

    if (addend2_is_imm) {
        const bool insn_has_offset = (insn->opcode < RTLOP_ATOMIC_INC);
        const int offset = insn_has_offset ? insn->offset : 0;
        const int64_t final_offset = (int64_t)addend2 + (int64_t)offset;
        if ((uint64_t)(final_offset+0x80000000u) < UINT64_C(0x100000000)) {
            insn->src1 = addend1;
            if (insn_has_offset) {
                insn->offset = 0;
            }
            insn->host_data_32 = (int32_t)final_offset;
        } else {
            return;  // Can't optimize.
        }
    } else {
        ASSERT(addend2 != 0xFFFF);
        insn->src1 = addend1;
        insn->host_data_16 = addend2;
    }

    if (unit->regs[addend1].death < insn_index) {
        unit->regs[addend1].death = insn_index;
    }
    if (!addend2_is_imm && unit->regs[addend2].death < insn_index) {
        unit->regs[addend2].death = insn_index;
    }

    src1_reg->death = src1_reg->birth;
    RTLInsn * const birth_insn = &unit->insns[src1_reg->birth];
    birth_insn->opcode = RTLOP_NOP;
    birth_insn->dest = 0;
    birth_insn->src1 = 0;
    birth_insn->src2 = 0;
    birth_insn->src_imm = 0;
}

/*************************************************************************/
/*************************************************************************/
