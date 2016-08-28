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

/* Macro to validate operand constraints, returning false from the current
 * function on constraint violation. */
#define OPERAND_ASSERT(constraint)  do {        \
    if (UNLIKELY(!(constraint))) {              \
        log_ice(unit->handle, "Operand constraint violated: %s", #constraint);\
        return false;                           \
    }                                           \
} while (0)

/* Macro to mark a given register live, updating birth/death fields too.
 * Requires unit and insn_index to be separately defined. */
#define MARK_LIVE(reg,index)  do {              \
    RTLRegister * const __reg = (reg);          \
    const unsigned int __index = (index);       \
    if (!__reg->live) {                         \
        __reg->live = 1;                        \
        __reg->birth = insn_index;              \
        if (!unit->last_live_reg) {             \
            unit->first_live_reg = __index;     \
        } else {                                \
            unit->regs[unit->last_live_reg].live_link = __index; \
        }                                       \
        unit->last_live_reg = __index;          \
        reg->live_link = 0;                     \
    }                                           \
    __reg->death = insn_index;                  \
} while (0)

/*************************************************************************/
/*************************** Encoding routines ***************************/
/*************************************************************************/

/**
 * make_nop:  Encode a NOP instruction.
 */
static bool make_nop(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                     uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(insn != NULL);

    insn->src_imm = other;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_set_alias:  Encode a SET_ALIAS instruction.
 */
static bool make_set_alias(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                           uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_alias);
    ASSERT(src1 != 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->alias_types[dest]);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const src1reg = &unit->regs[src1];
    const uint32_t insn_index = unit->num_insns;
    MARK_LIVE(src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_get_alias:  Encode a GET_ALIAS instruction.
 */
static bool make_get_alias(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                           uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_alias);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == unit->alias_types[src1]);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_ALIAS;
    destreg->alias.src = src1;
    MARK_LIVE(destreg, dest);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_move:  Encode a MOVE instruction.
 */
static bool make_move(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                      uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.src1 = src1;
    destreg->result.src2 = 0;
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_select:  Encode a SELECT instruction.
 */
static bool make_select(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                        uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);
    ASSERT(src2 != 0 && src2 < unit->next_reg);
    ASSERT(other != 0 && other < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[other].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[other].type == RTLTYPE_INT32
                   || unit->regs[other].type == RTLTYPE_ADDRESS);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;
    insn->cond = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    RTLRegister * const condreg = &unit->regs[other];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.cond = other;
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);
    MARK_LIVE(src2reg, src2);
    MARK_LIVE(condreg, other);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_alu_1op:  Encode a 1-operand ALU instruction.
 */
static bool make_alu_1op(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                         uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.src1 = src1;
    destreg->result.src2 = 0;
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_alu_2op:  Encode a 2-operand ALU instruction.
 */
static bool make_alu_2op(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                         uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);
    ASSERT(src2 != 0 && src2 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src2].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(unit->regs[src2].type == unit->regs[dest].type);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->src2 = src2;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    RTLRegister * const src2reg = &unit->regs[src2];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);
    MARK_LIVE(src2reg, src2);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_bitfield:  Encode a bitfield instruction.
 */
static bool make_bitfield(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                          uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    const int start = other & 0xFF;
    const int count = (other >> 8) & 0xFF;
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);
    ASSERT(insn->opcode != RTLOP_BFINS || (src2 != 0 && src2 < unit->next_reg));

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == unit->regs[dest].type);
    OPERAND_ASSERT(start < 32);
    OPERAND_ASSERT(count <= 32 - start);
    if (insn->opcode == RTLOP_BFINS) {
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
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_RESULT;
    destreg->result.opcode = insn->opcode;
    destreg->result.src1 = src1;
    destreg->result.src2 = src2;
    destreg->result.start = start;
    destreg->result.count = count;
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);
    if (src2) {
        MARK_LIVE(src2reg, src2);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load_imm:  Encode a LOAD_IMM instruction.
 */
static bool make_load_imm(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                          uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_INT32
                   || unit->regs[dest].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_ADDRESS
                   || other <= 0xFFFFFFFF);
#endif

    insn->dest = dest;
    insn->src_imm = other;

    RTLRegister * const destreg = &unit->regs[dest];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_CONSTANT;
    switch (unit->regs[dest].type) {
      case RTLTYPE_INT32:
        destreg->value.int32 = (uint32_t)other;
        break;
      case RTLTYPE_ADDRESS:
        destreg->value.address = other;
        break;
      default:
        UNREACHABLE;
    }
    MARK_LIVE(destreg, dest);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load_arg:  Encode a LOAD_ARG instruction.
 */
static bool make_load_arg(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                          uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_INT32
                   || unit->regs[dest].type == RTLTYPE_ADDRESS);
#endif

    insn->dest = dest;
    insn->src_imm = other;

    RTLRegister * const destreg = &unit->regs[dest];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_FUNC_ARG;
    destreg->arg_index = other;
    MARK_LIVE(destreg, dest);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_load:  Encode a memory load instruction.
 */
static bool make_load(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                      uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);

    /* Required data types for each instruction. */
    static const uint8_t type_lookup[] = {
        [RTLOP_LOAD_BU  ] = RTLTYPE_INT32,
        [RTLOP_LOAD_BS  ] = RTLTYPE_INT32,
        [RTLOP_LOAD_HU  ] = RTLTYPE_INT32,
        [RTLOP_LOAD_HS  ] = RTLTYPE_INT32,
        [RTLOP_LOAD_W   ] = RTLTYPE_INT32,
        [RTLOP_LOAD_ADDR] = RTLTYPE_ADDRESS,
    };
    /* Lookup tables for destreg->memory.{size,is_signed} */
    static const uint8_t size_lookup[] = {
        [RTLOP_LOAD_BU] = 1, [RTLOP_LOAD_BS] = 1,
        [RTLOP_LOAD_HU] = 2, [RTLOP_LOAD_HS] = 2,
        [RTLOP_LOAD_W ] = 4,
    };
    static const uint8_t is_signed_lookup[] = {
        [RTLOP_LOAD_BS] = 1,
        [RTLOP_LOAD_HS] = 1,
    };

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source == RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == type_lookup[insn->opcode]);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT((int64_t)other >= -0x8000 && (int64_t)other <= 0x7FFF);
#endif

    const unsigned int opcode = insn->opcode;

    insn->dest = dest;
    insn->src1 = src1;
    insn->offset = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const uint32_t insn_index = unit->num_insns;
    destreg->source = RTLREG_MEMORY;
    destreg->memory.addr_reg = src1;
    destreg->memory.offset = other;
    destreg->memory.size = size_lookup[opcode];
    destreg->memory.is_signed = is_signed_lookup[opcode];
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_store:  Encode a memory store instruction.
 */
static bool make_store(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                       uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(dest != 0 && dest < unit->next_reg);
    ASSERT(src1 != 0 && src1 < unit->next_reg);

    /* Required data types for each instruction. */
    static const uint8_t type_lookup[] = {
        [RTLOP_STORE_B   ] = RTLTYPE_INT32,
        [RTLOP_STORE_H   ] = RTLTYPE_INT32,
        [RTLOP_STORE_W   ] = RTLTYPE_INT32,
        [RTLOP_STORE_ADDR] = RTLTYPE_ADDRESS,
    };

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[dest].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[dest].type == RTLTYPE_ADDRESS);
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == type_lookup[insn->opcode]);
    OPERAND_ASSERT((int64_t)other >= -0x8000 && (int64_t)other <= 0x7FFF);
#endif

    insn->dest = dest;
    insn->src1 = src1;
    insn->offset = other;

    RTLRegister * const destreg = &unit->regs[dest];
    RTLRegister * const src1reg = &unit->regs[src1];
    const uint32_t insn_index = unit->num_insns;
    MARK_LIVE(destreg, dest);
    MARK_LIVE(src1reg, src1);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_label:  Encode a LABEL instruction.
 */
static bool make_label(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                       uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->cur_block < unit->num_blocks);
    ASSERT(unit->label_blockmap != NULL);
    ASSERT(insn != NULL);
    ASSERT(other != 0 && other < unit->next_label);

    insn->label = other;

    /* If this is _not_ the first instruction in the current basic block,
     * end it and create a new basic block starting here, since there could
     * potentially be a branch to this location. */
    if ((uint32_t)unit->blocks[unit->cur_block].first_insn != unit->num_insns) {
        unit->blocks[unit->cur_block].last_insn = unit->num_insns - 1;
        if (UNLIKELY(!rtl_block_add(unit))) {
            log_ice(unit->handle, "%u: Failed to start a new basic block",
                    unit->num_insns);
            return false;
        }
        const uint32_t new_block = unit->num_blocks - 1;
        if (UNLIKELY(!rtl_block_add_edge(unit, unit->cur_block, new_block))){
            log_ice(unit->handle, "%u: Failed to add edge %u->%u",
                    unit->num_insns, unit->cur_block, new_block);
            return false;
        }
        unit->cur_block = new_block;
        unit->blocks[unit->cur_block].first_insn = unit->num_insns;
    }

    /* Save the label's unit number in the label-to-unit map */
    unit->label_blockmap[insn->label] = unit->cur_block;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_goto:  Encode a GOTO instruction.
 */
static bool make_goto(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                      uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(other != 0 && other < unit->next_label);

    insn->label = other;

    /* Terminate the current basic block after this instruction. */
    unit->blocks[unit->cur_block].last_insn = unit->num_insns;
    unit->have_block = 0;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_goto_cond:  Encode a GOTO_IF_Z or GOTO_IF_NZ instruction.
 */
static bool make_goto_cond(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                           uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 != 0 && src1 < unit->next_reg);
    ASSERT(other != 0 && other < unit->next_label);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
    OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_INT32
                   || unit->regs[src1].type == RTLTYPE_ADDRESS);
#endif

    insn->src1 = src1;
    insn->label = other;

    RTLRegister * const src1reg = &unit->regs[src1];
    const uint32_t insn_index = unit->num_insns;
    MARK_LIVE(src1reg, src1);

    /* Terminate the current basic block after this instruction, and
     * start a new basic block with an edge connecting from this one. */
    unit->blocks[unit->cur_block].last_insn = unit->num_insns;
    if (UNLIKELY(!rtl_block_add(unit))) {
        log_ice(unit->handle, "%u: Failed to start a new basic block",
                unit->num_insns);
        return false;
    }
    const unsigned int new_block = unit->num_blocks - 1;
    if (UNLIKELY(!rtl_block_add_edge(unit, unit->cur_block, new_block))) {
        log_ice(unit->handle, "%u: Failed to add edge %u->%u",
                unit->num_insns, unit->cur_block, new_block);
        return false;
    }
    unit->cur_block = new_block;
    unit->blocks[unit->cur_block].first_insn = unit->num_insns + 1;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_return:  Encode a RETURN instruction.
 */
static bool make_return(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                        uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(insn != NULL);
    ASSERT(src1 < unit->next_reg);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    if (src1 != 0) {
        OPERAND_ASSERT(unit->regs[src1].source != RTLREG_UNDEFINED);
        OPERAND_ASSERT(unit->regs[src1].type == RTLTYPE_INT32
                       || unit->regs[src1].type == RTLTYPE_ADDRESS);
    }
#endif

    insn->src1 = src1;

    /* Terminate the current basic block, like GOTO. */
    unit->blocks[unit->cur_block].last_insn = unit->num_insns;
    unit->have_block = 0;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * make_illegal:  Encode an ILLEGAL instruction.
 */
static bool make_illegal(RTLUnit *unit, RTLInsn *insn, unsigned int dest,
                         uint32_t src1, uint32_t src2, uint64_t other)
{
    ASSERT(insn != NULL);

    return true;
}

/*************************************************************************/
/************************ Encoding function table ************************/
/*************************************************************************/

bool (* const makefunc_table[])(RTLUnit *, RTLInsn *, unsigned int,
                                uint32_t, uint32_t, uint64_t) = {
    [RTLOP_NOP       ] = make_nop,
    [RTLOP_SET_ALIAS ] = make_set_alias,
    [RTLOP_GET_ALIAS ] = make_get_alias,
    [RTLOP_MOVE      ] = make_move,
    [RTLOP_SELECT    ] = make_select,
    [RTLOP_ADD       ] = make_alu_2op,
    [RTLOP_SUB       ] = make_alu_2op,
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
    [RTLOP_SLL       ] = make_alu_2op,
    [RTLOP_SRL       ] = make_alu_2op,
    [RTLOP_SRA       ] = make_alu_2op,
    [RTLOP_ROR       ] = make_alu_2op,
    [RTLOP_CLZ       ] = make_alu_1op,
    [RTLOP_SLTU      ] = make_alu_2op,
    [RTLOP_SLTS      ] = make_alu_2op,
    [RTLOP_BSWAP     ] = make_alu_1op,
    [RTLOP_BFEXT     ] = make_bitfield,
    [RTLOP_BFINS     ] = make_bitfield,
    [RTLOP_LOAD_IMM  ] = make_load_imm,
    [RTLOP_LOAD_ARG  ] = make_load_arg,
    [RTLOP_LOAD_BS   ] = make_load,
    [RTLOP_LOAD_BU   ] = make_load,
    [RTLOP_LOAD_HS   ] = make_load,
    [RTLOP_LOAD_HU   ] = make_load,
    [RTLOP_LOAD_W    ] = make_load,
    [RTLOP_LOAD_ADDR ] = make_load,
    [RTLOP_STORE_B   ] = make_store,
    [RTLOP_STORE_H   ] = make_store,
    [RTLOP_STORE_W   ] = make_store,
    [RTLOP_STORE_ADDR] = make_store,
    [RTLOP_LABEL     ] = make_label,
    [RTLOP_GOTO      ] = make_goto,
    [RTLOP_GOTO_IF_Z ] = make_goto_cond,
    [RTLOP_GOTO_IF_NZ] = make_goto_cond,
    [RTLOP_RETURN    ] = make_return,
    [RTLOP_ILLEGAL   ] = make_illegal,
};

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
