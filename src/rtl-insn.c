/*
 * libbinjit: a just-in-time recompiler for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"

/*
 * This source file defines functions for encoding RTL instructions into
 * the RTLInsn structure based on the type of instruction (2-operand ALU,
 * memory load, and so on).  These functions are not exported directly, but
 * are instead called through a lookup table by rtl_insn_make(), an inline
 * function defined in rtl-internal.h.
 *
 * Both the inlining of rtl_insn_make() and the requirement of preloading
 * the opcode into insn->opcode (and then taking the RTLInsn pointer in
 * place of the opcode parameter to rtl_add_insn()) are to help
 * optimization of the fast path; for example, on MIPS architectures,
 * rtl_add_insn() can directly call the encoding function in this file
 * without having to pass through a call to rtl_insn_make() or reload the
 * parameter registers ($a0-$t1).
 */

/*************************************************************************/
/************************** Local declarations ***************************/
/*************************************************************************/

/**
 * OPERAND_ASSERT:  Validate operand constraints, returning false from the
 * current function on constraint violation.
 */
#define OPERAND_ASSERT(constraint)  do {        \
    if (UNLIKELY(!(constraint))) {              \
        log_ice(unit->handle, "Operand constraint violated: %s", #constraint);\
        return false;                           \
    }                                           \
} while (0)

/*-----------------------------------------------------------------------*/

/**
 * rtl_mark_live:  Mark the given register live, also updating the birth
 * and death fields as appropriate.
 *
 * [Parameters]
 *     unit: RTLUnit into which instruction is being inserted.
 *     insn_index: Index of instruction in unit->insns[].
 *     reg: Pointer to RTLRegister structure for register.
 *     index: Register index.
 */
static ALWAYS_INLINE void rtl_mark_live(
    RTLUnit * const unit, const int insn_index, RTLRegister * const reg,
    const int index)
{
    if (!reg->live) {
        reg->live = true;
        reg->birth = insn_index;
        if (!unit->first_live_reg) {
            unit->first_live_reg = index;
        }
        unit->last_live_reg = index;
    }
    reg->death = insn_index;
    unit->blocks[unit->cur_block].max_live_reg =
        max(unit->blocks[unit->cur_block].max_live_reg, index);
}

/*************************************************************************/
/*************************** Encoding routines ***************************/
/*************************************************************************/

/**
 * make_0op:  Encode an instruction which takes no operands.
 */
static bool make_0op(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                     int src2, uint64_t other)
{
    return true;  // Nothing to encode.
}

/*-----------------------------------------------------------------------*/

/**
 * make_nop:  Encode a NOP instruction.
 */
static bool make_nop(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                     int src2, uint64_t other)
{
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    if (dest != 0) {
        OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    }
    if (src1 != 0) {
        OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    }
    if (src2 != 0) {
        OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    }
    OPERAND_ASSERT(!(src1 == 0 && src2 != 0));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->src_imm = other;

    const int insn_index = unit->num_insns;
    if (dest) {
        RTLRegister * const destreg = &unit->regs[dest];
        unit->regs[dest].source = RTLREG_RESULT_NOFOLD;
        unit->regs[dest].result.opcode = RTLOP_NOP;
        rtl_mark_live(unit, insn_index, destreg, dest);
    }
    if (src1) {
        RTLRegister * const src1reg = &unit->regs[src1];
        rtl_mark_live(unit, insn_index, src1reg, src1);
    }
    if (src2) {
        RTLRegister * const src2reg = &unit->regs[src2];
        rtl_mark_live(unit, insn_index, src2reg, src2);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_set_alias:  Encode a SET_ALIAS instruction.
 */
static bool make_set_alias(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(other < unit->next_alias);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->aliases[other].type);
#endif

    insn->src1 = src1;
    insn->alias = (uint16_t)other;

    RTLRegister * const src1reg = &unit->regs[src1];
    RTLAlias * const alias = &unit->aliases[other];
    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, src1reg, src1);
    if (alias->base) {
        rtl_mark_live(unit, insn_index, &unit->regs[alias->base], alias->base);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_get_alias:  Encode a GET_ALIAS instruction.
 */
static bool make_get_alias(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(other < unit->next_alias);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == unit->aliases[other].type);
#endif

    insn->dest = dest;
    insn->alias = (uint16_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLAlias * const alias = &unit->aliases[other];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_ALIAS;
    destreg->alias.src = (uint16_t)other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    if (alias->base) {
        rtl_mark_live(unit, insn_index, &unit->regs[alias->base], alias->base);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_move:  Encode a MOVE instruction.
 */
static bool make_move(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                      int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_select:  Encode a SELECT instruction.
 */
static bool make_select(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                        int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);
    ASSERT(other < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[other].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[other]));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->src3 = (uint16_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    RTLRegister * const src3reg = &unit->regs[other];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.src3 = (uint16_t)other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);
    rtl_mark_live(unit, insn_index, src3reg, other);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_intcast:  Encode an integer size-cast instruction.
 */
static bool make_intcast(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_alu_1op:  Encode a 1-operand ALU-type instruction.
 */
static bool make_alu_1op(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_alu_2op:  Encode a 2-operand ALU-type instruction.
 */
static bool make_alu_2op(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_shift:  Encode a shift/rotate instruction.
 */
static bool make_shift(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                       int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src2]));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_alu_imm:  Encode a register-immediate ALU-type instruction.
 */
static bool make_alu_imm(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_INT32
                   || other + UINT64_C(0x80000000) < UINT64_C(0x100000000));
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src_imm = (int32_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 1;
    destreg->result.src1 = src1;
    destreg->result.src_imm = (int32_t)other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_clz:  Encode a CLZ instruction.
 */
static bool make_clz(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                     int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_cmp:  Encode a comparison instruction.
 */
static bool make_cmp(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                     int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[src1].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_cmp_imm:  Encode a register-immediate comparison instruction.
 */
static bool make_cmp_imm(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_INT32
                   || other + UINT64_C(0x80000000) < UINT64_C(0x100000000));
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src_imm = (int32_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 1;
    destreg->result.src1 = src1;
    destreg->result.src_imm = (int32_t)other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_bitfield:  Encode a bitfield instruction.
 */
static bool make_bitfield(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                          int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

    const int start = other & 0xFF;
    const int count = (other >> 8) & 0xFF;

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    if (insn->opcode == RTLOP_BFINS) {
        OPERAND_ASSERT(src2 != 0);
        OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
        OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
    }
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->bitfield.start = start;
    insn->bitfield.count = count;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.start = start;
    destreg->result.count = count;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    if (src2) {
        rtl_mark_live(unit, insn_index, src2reg, src2);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_bitcast:  Encode a BITCAST instruction.
 */
static bool make_bitcast(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT((unit->regs[dest].type == RTLTYPE_INT32
                    && unit->regs[src1].type == RTLTYPE_FLOAT32)
                || (unit->regs[dest].type == RTLTYPE_INT64
                    && unit->regs[src1].type == RTLTYPE_FLOAT64)
                || (unit->regs[dest].type == RTLTYPE_FLOAT32
                    && unit->regs[src1].type == RTLTYPE_INT32)
                || (unit->regs[dest].type == RTLTYPE_FLOAT64
                    && unit->regs[src1].type == RTLTYPE_INT64));
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fcast:  Encode an FCAST or FCVT instruction.
 */
static bool make_fcast(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                       int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[src1]));
    OPERAND_ASSERT(insn->opcode != RTLOP_FCVT
                   || unit->regs[src1].type != unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_ficast:  Encode an integer-to-float cast instruction (FZCAST or
 * FSCAST).
 */
static bool make_ficast(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                        int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_froundi:  Encode a float-to-integer rounding instruction (FROUNDI
 * or FTRUNCI).
 */
static bool make_froundi(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[src1]));
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fpu_1op:  Encode a 1-operand floating-point instruction.
 */
static bool make_fpu_1op(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    /* We currently don't have integer vector types, so we don't need to
     * explicitly check that vectors have floating-point elements. */
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[dest])
                || rtl_register_is_vector(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fpu_2op:  Encode a 2-operand floating-point instruction.
 */
static bool make_fpu_2op(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[dest])
                || rtl_register_is_vector(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fpu_3op:  Encode a 3-operand floating-point instruction.
 */
static bool make_fpu_3op(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);
    ASSERT(other < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[other].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[dest])
                || rtl_register_is_vector(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[other].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->src3 = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    RTLRegister * const src3reg = &unit->regs[other];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.src3 = other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);
    rtl_mark_live(unit, insn_index, src3reg, other);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fcmp:  Encode an FCMP instruction.
 */
static bool make_fcmp(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                      int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(rtl_register_is_float(&unit->regs[src1]));
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[src1].type);
    OPERAND_ASSERT(other <= 31);
    OPERAND_ASSERT((other & 7) <= RTLFCMP_UN);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->fcmp = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.fcmp = other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fgetstate:  Encode an FGETSTATE instruction.
 */
static bool make_fgetstate(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_FPSTATE);
#endif

    insn->dest = dest;

    RTLRegister * const destreg = &unit->regs[dest];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    rtl_mark_live(unit, insn_index, destreg, dest);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fsetstate:  Encode an FSETSTATE instruction.
 */
static bool make_fsetstate(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_FPSTATE);
#endif

    insn->src1 = src1;

    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_ftestexc:  Encode an FTESTEXC instruction.
 */
static bool make_ftestexc(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                          int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_FPSTATE);
    OPERAND_ASSERT(other <= RTLFEXC_ZERO_DIVIDE);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src_imm = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 1;
    destreg->result.src1 = src1;
    destreg->result.src_imm = other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fclearexc:  Encode an FCLEAREXC instruction.
 */
static bool make_fclearexc(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_FPSTATE);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_FPSTATE);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fsetround:  Encode an FSETROUND instruction.
 */
static bool make_fsetround(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_FPSTATE);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_FPSTATE);
    OPERAND_ASSERT(other <= RTLFROUND_CEIL);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src_imm = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 1;
    destreg->result.src1 = src1;
    destreg->result.src_imm = other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_fcopyround:  Encode an FCOPYROUND instruction.
 */
static bool make_fcopyround(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                            int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_FPSTATE);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_FPSTATE);
    OPERAND_ASSERT(unit->regs[src2].type == RTLTYPE_FPSTATE);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_vbuild2:  Encode a VBUILD2 instruction.
 */
static bool make_vbuild2(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_vector(&unit->regs[dest]));
    /* Currently we only have 2-element vector types, so this test will
     * always be true. */
    ASSERT(rtl_vector_length(unit->regs[dest].type) == 2);
    OPERAND_ASSERT(unit->regs[src1].type
                   == rtl_vector_element_type(unit->regs[dest].type));
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[src1].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_vbroadcast:  Encode a VBROADCAST instruction.
 */
static bool make_vbroadcast(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                            int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_vector(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type
                   == rtl_vector_element_type(unit->regs[dest].type));
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_vextract:  Encode a VEXTRACT instruction.
 */
static bool make_vextract(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                          int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_vector(&unit->regs[src1]));
    OPERAND_ASSERT(unit->regs[dest].type
                   == rtl_vector_element_type(unit->regs[src1].type));
    OPERAND_ASSERT(other <
                   (unsigned int)rtl_vector_length(unit->regs[src1].type));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->elem = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.elem = other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_vinsert:  Encode a VINSERT instruction.
 */
static bool make_vinsert(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_vector(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type
                   == rtl_vector_element_type(unit->regs[dest].type));
    OPERAND_ASSERT(other <
                   (unsigned int)rtl_vector_length(unit->regs[dest].type));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->elem = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.elem = other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_vfcast:  Encode a VFCAST or VFCVT instruction.
 */
static bool make_vfcast(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                        int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_vector(&unit->regs[dest]));
    /* Currently we don't have any integer vector types, so this test will
     * always be true. */
    ASSERT(rtl_type_is_float(rtl_vector_element_type(unit->regs[dest].type)));
    OPERAND_ASSERT(rtl_register_is_vector(&unit->regs[src1]));
    ASSERT(rtl_type_is_float(rtl_vector_element_type(unit->regs[src1].type)));
    OPERAND_ASSERT(insn->opcode != RTLOP_VFCVT
                   || unit->regs[src1].type != unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load_imm:  Encode a LOAD_IMM instruction.
 */
static bool make_load_imm(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                          int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_scalar(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_INT64
                   || unit->regs[dest].type == RTLTYPE_ADDRESS
                   || unit->regs[dest].type == RTLTYPE_FLOAT64
                   || (unit->regs[dest].type == RTLTYPE_INT32 && other >= UINT64_C(-0x80000000))
                   || other <= UINT64_C(0xFFFFFFFF));
#endif

    insn->dest = dest;
    if (unit->regs[dest].type == RTLTYPE_INT32
     || unit->regs[dest].type == RTLTYPE_FLOAT32) {
        insn->src_imm = (uint32_t)other;
    } else {
        insn->src_imm = other;
    }

    RTLRegister * const destreg = &unit->regs[dest];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_CONSTANT;
    if (unit->regs[dest].type == RTLTYPE_INT32
     || unit->regs[dest].type == RTLTYPE_FLOAT32) {
        destreg->value.i64 = (uint32_t)other;
    } else {
        destreg->value.i64 = other;
    }
    rtl_mark_live(unit, insn_index, destreg, dest);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load_arg:  Encode a LOAD_ARG instruction.
 */
static bool make_load_arg(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                          int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
#endif

    insn->dest = dest;
    insn->arg_index = (uint8_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_FUNC_ARG;
    destreg->arg_index = (uint8_t)other;
    rtl_mark_live(unit, insn_index, destreg, dest);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load:  Encode a memory load instruction.
 */
static bool make_load(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                      int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(other <= 0x7FFF || other >= UINT64_C(-0x8000));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->offset = (int16_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_MEMORY;
    destreg->memory.base = src1;
    destreg->memory.offset = (int16_t)other;
    destreg->memory.byterev = (insn->opcode == RTLOP_LOAD_BR);
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load_narrow:  Encode a memory load instruction for a narrow integer.
 */
static bool make_load_narrow(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                             int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

    /* Lookup tables for each instruction. */
    static const struct {
        uint8_t size;   // destreg->memory.size for this insn
        bool is_signed; // destreg->memory.is_signed for this insn
    } insn_info[] = {
        [RTLOP_LOAD_U8     - RTLOP_LOAD_U8] = {.size = 1, .is_signed = false},
        [RTLOP_LOAD_S8     - RTLOP_LOAD_U8] = {.size = 1, .is_signed = true},
        [RTLOP_LOAD_U16    - RTLOP_LOAD_U8] = {.size = 2, .is_signed = false},
        [RTLOP_LOAD_S16    - RTLOP_LOAD_U8] = {.size = 2, .is_signed = true},
        [RTLOP_LOAD_U16_BR - RTLOP_LOAD_U8] = {.size = 2, .is_signed = false},
        [RTLOP_LOAD_S16_BR - RTLOP_LOAD_U8] = {.size = 2, .is_signed = true},
    };

    const int lookup_index = insn->opcode - RTLOP_LOAD_U8;
    ASSERT(lookup_index >= 0 && lookup_index < lenof(insn_info));
    ASSERT(insn_info[lookup_index].size > 0);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_INT32);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(other <= 0x7FFF || other >= UINT64_C(-0x8000));
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->offset = (int16_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_MEMORY;
    destreg->memory.base = src1;
    destreg->memory.offset = (int16_t)other;
    destreg->memory.byterev = (insn->opcode >= RTLOP_LOAD_U16_BR);
    destreg->memory.size = insn_info[lookup_index].size;
    destreg->memory.is_signed = insn_info[lookup_index].is_signed;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_store:  Encode a memory store instruction.
 */
static bool make_store(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                       int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(other <= 0x7FFF || other >= UINT64_C(-0x8000));
#endif

    insn->src1 = src1;
    insn->src2 = src2;
    insn->offset = (int16_t)other;

    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_store_narrow:  Encode a memory store instruction.
 */
static bool make_store_narrow(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                              int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].type == RTLTYPE_INT32);
    OPERAND_ASSERT(other <= 0x7FFF || other >= UINT64_C(-0x8000));
#endif

    insn->src1 = src1;
    insn->src2 = src2;
    insn->offset = (int16_t)other;

    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_atomic_inc:  Encode an ATOMIC_INC instruction.
 */
static bool make_atomic_inc(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                            int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->offset = (int16_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_cmpxchg:  Encode a CMPXCHG instruction.
 */
static bool make_cmpxchg(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                         int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);
    ASSERT(other < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(dest != 0);
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[other].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[other].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->src3 = (uint16_t)other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    RTLRegister * const src3reg = &unit->regs[other];
    const int insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT_NOFOLD;
    destreg->result.opcode = insn->opcode;
    destreg->result.is_imm = 0;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.src3 = (uint16_t)other;
    rtl_mark_live(unit, insn_index, destreg, dest);
    rtl_mark_live(unit, insn_index, src1reg, src1);
    rtl_mark_live(unit, insn_index, src2reg, src2);
    rtl_mark_live(unit, insn_index, src3reg, other);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_label:  Encode a LABEL instruction.
 */
static bool make_label(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                       int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->cur_block < unit->num_blocks);
    ASSERT(unit->label_blockmap != NULL);
    ASSERT(insn != NULL);
    ASSERT(other < unit->next_label);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->label_blockmap[other] < 0);
#endif

    insn->label = (uint16_t)other;

    /* If this is _not_ the first instruction in the current basic block,
     * end it and create a new basic block starting here, since there could
     * potentially be a branch to this location. */
    if ((uint32_t)unit->blocks[unit->cur_block].first_insn != unit->num_insns) {
        unit->blocks[unit->cur_block].last_insn = unit->num_insns - 1;
        if (UNLIKELY(!rtl_block_add(unit))) {
            log_ice(unit->handle, "Failed to start a new basic block at %u",
                    unit->num_insns);
            return false;
        }
        const uint32_t new_block = unit->num_blocks - 1;
        /* The only ways rtl_block_add_edge() can fail is if the block
         * already has two outgoing edges or the new block is full of
         * incoming edges.  We never add outgoing edges except when
         * terminating a block, and the new block will have no incoming
         * edges, so this call will always succeed. */
        ASSERT(rtl_block_add_edge(unit, unit->cur_block, new_block));
        unit->cur_block = new_block;
        unit->blocks[unit->cur_block].first_insn = unit->num_insns;
    }

    /* Save the label's unit number in the label-to-unit map. */
    unit->label_blockmap[insn->label] = unit->cur_block;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_goto:  Encode a GOTO instruction.
 */
static bool make_goto(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                      int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(other < unit->next_label);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(other != 0);
#endif

    insn->label = (uint16_t)other;

    /* Terminate the current basic block after this instruction. */
    unit->blocks[unit->cur_block].last_insn = unit->num_insns;
    unit->have_block = false;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_goto_cond:  Encode a GOTO_IF_Z or GOTO_IF_NZ instruction.
 */
static bool make_goto_cond(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                           int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(other < unit->next_label);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(other != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
#endif

    insn->src1 = src1;
    insn->label = (uint16_t)other;

    RTLRegister * const src1reg = &unit->regs[src1];
    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, src1reg, src1);

    /* Terminate the current basic block after this instruction, and
     * start a new basic block with an edge connecting from this one. */
    unit->blocks[unit->cur_block].last_insn = unit->num_insns;
    if (UNLIKELY(!rtl_block_add(unit))) {
        log_ice(unit->handle, "Failed to start a new basic block at %u",
                unit->num_insns);
        return false;
    }
    const unsigned int new_block = unit->num_blocks - 1;
    /* See note in make_label() for why this can never fail. */
    ASSERT(rtl_block_add_edge(unit, unit->cur_block, new_block));
    unit->cur_block = new_block;
    unit->blocks[unit->cur_block].first_insn = unit->num_insns + 1;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_call:  Encode a CALL instruction.
 */
static bool make_call(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                      int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest >= 0 && dest < unit->next_reg);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);
    ASSERT(other < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    if (dest != 0) {
        OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
        OPERAND_ASSERT(rtl_register_is_int(&unit->regs[dest]));
    }
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(!(src2 == 0 && other != 0));
    if (src2 != 0) {
        OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
        OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src2]));
        if (other != 0) {
            OPERAND_ASSERT(unit->regs[other].source != RTLREG_UNDEFINED);
            OPERAND_ASSERT(rtl_register_is_int(&unit->regs[other]));
        }
    }
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->src3 = other;

    const int insn_index = unit->num_insns;
    if (dest) {
        RTLRegister * const destreg = &unit->regs[dest];
        destreg->source = RTLREG_RESULT_NOFOLD;
        destreg->result.opcode = insn->opcode;
        destreg->result.is_imm = 0;
        destreg->result.src1 = src1;
        destreg->result.src2 = src2;
        destreg->result.src3 = (uint16_t)other;
        rtl_mark_live(unit, insn_index, destreg, dest);
    }
    rtl_mark_live(unit, insn_index, &unit->regs[src1], src1);
    if (src2) {
        rtl_mark_live(unit, insn_index, &unit->regs[src2], src2);
        if (other) {
            rtl_mark_live(unit, insn_index, &unit->regs[other], other);
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_return:  Encode a RETURN instruction.
 */
static bool make_return(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                        int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    if (src1 != 0) {
        OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
        OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
    }
#endif

    insn->src1 = src1;

    if (src1) {
        RTLRegister * const src1reg = &unit->regs[src1];
        const int insn_index = unit->num_insns;
        rtl_mark_live(unit, insn_index, src1reg, src1);
    }

    /* Terminate the current basic block, like GOTO. */
    unit->blocks[unit->cur_block].last_insn = unit->num_insns;
    unit->have_block = false;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_chain:  Encode a CHAIN instruction.
 */
static bool make_chain(RTLUnit *unit, RTLInsn *insn, int dest, int src1,
                       int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(src2 >= 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(src2 != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src1]));
    OPERAND_ASSERT(rtl_register_is_int(&unit->regs[src2]));
#endif

    insn->src1 = src1;
    insn->src2 = src2;

    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, &unit->regs[src1], src1);
    rtl_mark_live(unit, insn_index, &unit->regs[src2], src2);

    /* Properly speaking, a CHAIN should terminate its basic block since
     * it's effectively a conditional branch out of the unit, but we only
     * use it in very specific circumstances which don't suffer from not
     * terminating the block, so we don't bother. */

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_chain_resolve:  Encode a CHAIN_RESOLVE instruction.
 */
static bool make_chain_resolve(RTLUnit *unit, RTLInsn *insn, int dest,
                               int src1, int src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 >= 0 && src1 < unit->next_reg);
    ASSERT(other < unit->num_insns);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(src1 != 0);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(unit->insns[other].opcode == RTLOP_CHAIN);
#endif

    insn->src1 = src1;
    insn->src_imm = other;

    const int insn_index = unit->num_insns;
    rtl_mark_live(unit, insn_index, &unit->regs[src1], src1);

    return true;
}

/*************************************************************************/
/************************ Encoding function table ************************/
/*************************************************************************/

bool (* const makefunc_table[])(RTLUnit *, RTLInsn *, int, int, int,
                                uint64_t) = {
    [RTLOP_NOP       ] = make_nop,
    [RTLOP_SET_ALIAS ] = make_set_alias,
    [RTLOP_GET_ALIAS ] = make_get_alias,
    [RTLOP_MOVE      ] = make_move,
    [RTLOP_SELECT    ] = make_select,
    [RTLOP_SCAST     ] = make_intcast,
    [RTLOP_ZCAST     ] = make_intcast,
    [RTLOP_SEXT8     ] = make_intcast,
    [RTLOP_SEXT16    ] = make_intcast,
    [RTLOP_ADD       ] = make_alu_2op,
    [RTLOP_SUB       ] = make_alu_2op,
    [RTLOP_NEG       ] = make_alu_1op,
    [RTLOP_MUL       ] = make_alu_2op,
    [RTLOP_MULHU     ] = make_alu_2op,
    [RTLOP_MULHS     ] = make_alu_2op,
    [RTLOP_DIVU      ] = make_alu_2op,
    [RTLOP_DIVS      ] = make_alu_2op,
    [RTLOP_MODU      ] = make_alu_2op,
    [RTLOP_MODS      ] = make_alu_2op,
    [RTLOP_AND       ] = make_alu_2op,
    [RTLOP_OR        ] = make_alu_2op,
    [RTLOP_XOR       ] = make_alu_2op,
    [RTLOP_NOT       ] = make_alu_1op,
    [RTLOP_SLL       ] = make_shift,
    [RTLOP_SRL       ] = make_shift,
    [RTLOP_SRA       ] = make_shift,
    [RTLOP_ROL       ] = make_shift,
    [RTLOP_ROR       ] = make_shift,
    [RTLOP_CLZ       ] = make_clz,
    [RTLOP_BSWAP     ] = make_alu_1op,
    [RTLOP_SEQ       ] = make_cmp,
    [RTLOP_SLTU      ] = make_cmp,
    [RTLOP_SLTS      ] = make_cmp,
    [RTLOP_SGTU      ] = make_cmp,
    [RTLOP_SGTS      ] = make_cmp,
    [RTLOP_BFEXT     ] = make_bitfield,
    [RTLOP_BFINS     ] = make_bitfield,
    [RTLOP_ADDI      ] = make_alu_imm,
    [RTLOP_MULI      ] = make_alu_imm,
    [RTLOP_ANDI      ] = make_alu_imm,
    [RTLOP_ORI       ] = make_alu_imm,
    [RTLOP_XORI      ] = make_alu_imm,
    [RTLOP_SLLI      ] = make_alu_imm,
    [RTLOP_SRLI      ] = make_alu_imm,
    [RTLOP_SRAI      ] = make_alu_imm,
    [RTLOP_RORI      ] = make_alu_imm,
    [RTLOP_SEQI      ] = make_cmp_imm,
    [RTLOP_SLTUI     ] = make_cmp_imm,
    [RTLOP_SLTSI     ] = make_cmp_imm,
    [RTLOP_SGTUI     ] = make_cmp_imm,
    [RTLOP_SGTSI     ] = make_cmp_imm,
    [RTLOP_BITCAST   ] = make_bitcast,
    [RTLOP_FCAST     ] = make_fcast,
    [RTLOP_FCVT      ] = make_fcast,
    [RTLOP_FZCAST    ] = make_ficast,
    [RTLOP_FSCAST    ] = make_ficast,
    [RTLOP_FROUNDI   ] = make_froundi,
    [RTLOP_FTRUNCI   ] = make_froundi,
    [RTLOP_FNEG      ] = make_fpu_1op,
    [RTLOP_FABS      ] = make_fpu_1op,
    [RTLOP_FNABS     ] = make_fpu_1op,
    [RTLOP_FADD      ] = make_fpu_2op,
    [RTLOP_FSUB      ] = make_fpu_2op,
    [RTLOP_FMUL      ] = make_fpu_2op,
    [RTLOP_FDIV      ] = make_fpu_2op,
    [RTLOP_FSQRT     ] = make_fpu_1op,
    [RTLOP_FCMP      ] = make_fcmp,
    [RTLOP_FMADD     ] = make_fpu_3op,
    [RTLOP_FMSUB     ] = make_fpu_3op,
    [RTLOP_FNMADD    ] = make_fpu_3op,
    [RTLOP_FNMSUB    ] = make_fpu_3op,
    [RTLOP_FGETSTATE ] = make_fgetstate,
    [RTLOP_FSETSTATE ] = make_fsetstate,
    [RTLOP_FTESTEXC  ] = make_ftestexc,
    [RTLOP_FCLEAREXC ] = make_fclearexc,
    [RTLOP_FSETROUND ] = make_fsetround,
    [RTLOP_FCOPYROUND] = make_fcopyround,
    [RTLOP_VBUILD2   ] = make_vbuild2,
    [RTLOP_VBROADCAST] = make_vbroadcast,
    [RTLOP_VEXTRACT  ] = make_vextract,
    [RTLOP_VINSERT   ] = make_vinsert,
    [RTLOP_VFCAST    ] = make_vfcast,
    [RTLOP_VFCVT     ] = make_vfcast,
    [RTLOP_LOAD_IMM  ] = make_load_imm,
    [RTLOP_LOAD_ARG  ] = make_load_arg,
    [RTLOP_LOAD      ] = make_load,
    [RTLOP_LOAD_U8   ] = make_load_narrow,
    [RTLOP_LOAD_S8   ] = make_load_narrow,
    [RTLOP_LOAD_U16  ] = make_load_narrow,
    [RTLOP_LOAD_S16  ] = make_load_narrow,
    [RTLOP_STORE     ] = make_store,
    [RTLOP_STORE_I8  ] = make_store_narrow,
    [RTLOP_STORE_I16 ] = make_store_narrow,
    [RTLOP_LOAD_BR   ] = make_load,
    [RTLOP_LOAD_U16_BR] = make_load_narrow,
    [RTLOP_LOAD_S16_BR] = make_load_narrow,
    [RTLOP_STORE_BR  ] = make_store,
    [RTLOP_STORE_I16_BR] = make_store_narrow,
    [RTLOP_ATOMIC_INC] = make_atomic_inc,
    [RTLOP_CMPXCHG   ] = make_cmpxchg,
    [RTLOP_LABEL     ] = make_label,
    [RTLOP_GOTO      ] = make_goto,
    [RTLOP_GOTO_IF_Z ] = make_goto_cond,
    [RTLOP_GOTO_IF_NZ] = make_goto_cond,
    [RTLOP_CALL      ] = make_call,
    [RTLOP_CALL_TRANSPARENT] = make_call,
    [RTLOP_RETURN    ] = make_return,
    [RTLOP_CHAIN     ] = make_chain,
    [RTLOP_CHAIN_RESOLVE] = make_chain_resolve,
    [RTLOP_ILLEGAL   ] = make_0op,
};

/*************************************************************************/
/*********************** Other interface routines ************************/
/*************************************************************************/

bool rtl_add_insn_copy(RTLUnit *unit, const RTLInsn *insn)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);
    ASSERT(insn);

    if (!rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0)) {
        return false;
    }

    const int insn_index = unit->num_insns - 1;
    unit->insns[insn_index] = *insn;
    if (insn->dest) {
        RTLRegister * const destreg = &unit->regs[insn->dest];
        rtl_mark_live(unit, insn_index, destreg, insn->dest);
    }
    if (insn->src1) {
        RTLRegister * const src1reg = &unit->regs[insn->src1];
        rtl_mark_live(unit, insn_index, src1reg, insn->src1);
    }
    if (insn->src2) {
        RTLRegister * const src2reg = &unit->regs[insn->src2];
        rtl_mark_live(unit, insn_index, src2reg, insn->src2);
    }
    if (rtl_opcode_has_src3(insn->opcode) && insn->src3) {
        RTLRegister * const src3reg = &unit->regs[insn->src3];
        rtl_mark_live(unit, insn_index, src3reg, insn->src3);
    }

    return true;
}

/*************************************************************************/
/*************************************************************************/
