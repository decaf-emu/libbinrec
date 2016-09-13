/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * add_block_edges:  Add edges between basic blocks for GOTO instructions.
 *
 * Execution time is O(n) in the number of basic blocks.
 *
 * [Parameters]
 *     unit: RTL unit.
 * [Return value]
 *     True on success, false on error.
 */
static bool add_block_edges(RTLUnit * const unit)
{
    ASSERT(unit != NULL);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->label_blockmap != NULL);

    unsigned int block_index;
    for (block_index = 0; block_index < unit->num_blocks; block_index++) {
        RTLBlock * const block = &unit->blocks[block_index];
        if (block->first_insn <= block->last_insn) {
            const RTLInsn * const insn = &unit->insns[block->last_insn];
            if (insn->opcode == RTLOP_GOTO
             || insn->opcode == RTLOP_GOTO_IF_Z
             || insn->opcode == RTLOP_GOTO_IF_NZ) {
                const unsigned int label = insn->label;
                if (UNLIKELY(unit->label_blockmap[label] < 0)) {
                    log_error(unit->handle, "GOTO to undefined label L%u at"
                              " %u", label, block->last_insn);
                    return false;
                } else if (UNLIKELY(!rtl_block_add_edge(unit, block_index, unit->label_blockmap[label]))) {
                    static const char * const opcode_names[] = {
                        [RTLOP_GOTO       - RTLOP_GOTO] = "GOTO",
                        [RTLOP_GOTO_IF_Z  - RTLOP_GOTO] = "GOTO_IF_Z",
                        [RTLOP_GOTO_IF_NZ - RTLOP_GOTO] = "GOTO_IF_NZ",
                    };
                    log_ice(unit->handle, "Failed to add edge %u->%u for"
                            " %s L%u at %u",
                            block_index, unit->label_blockmap[label],
                            opcode_names[insn->opcode - RTLOP_GOTO], label,
                            block->last_insn);
                    return false;
                }
            }
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * update_live_ranges:  Update the live range of any register live at the
 * beginning block targeted by a backward branch so that the register is
 * live through all branches that target the block.
 *
 * Worst-case execution time is O(n*m) in the number of blocks (n) and the
 * number of registers (m).  However, the register scan is only required
 * for blocks targeted by backward branches, and terminates at the first
 * register born within or after the targeted block.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
static void update_live_ranges(RTLUnit * const unit)
{
    ASSERT(unit != NULL);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);

    int block_index;
    for (block_index = 0; block_index < unit->num_blocks; block_index++) {
        const RTLBlock * const block = &unit->blocks[block_index];
        int latest_entry_block = -1;
        unsigned int i;
        for (i = 0; i < lenof(block->entries) && block->entries[i] >= 0; i++) {
            if (block->entries[i] > latest_entry_block) {
                latest_entry_block = block->entries[i];
            }
        }
        if (latest_entry_block >= block_index
         && unit->blocks[latest_entry_block].last_insn >= 0) {  // Just in case
            const int32_t birth_limit = block->first_insn;
            const int32_t min_death =
                unit->blocks[latest_entry_block].last_insn;
            unsigned int reg;
            for (reg = unit->first_live_reg;
                 reg != 0 && unit->regs[reg].birth < birth_limit;
                 reg = unit->regs[reg].live_link)
            {
                if (unit->regs[reg].death >= birth_limit
                 && unit->regs[reg].death < min_death) {
                    unit->regs[reg].death = min_death;
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_add_insn_with_extend:  Extend the instruction array and then add a
 * new instruction.  Called by rtl_add_insn() when the instruction array is
 * full upon entry.
 *
 * This function is explicitly NOINLINE to avoid having to spill the
 * function parameter registers on the rtl_add_insn() fast path.
 */
static NOINLINE bool rtl_add_insn_with_extend(
    RTLUnit *unit, RTLOpcode opcode, uint32_t dest, uint32_t src1,
    uint32_t src2, uint64_t other)
{
    uint32_t new_insns_size = unit->num_insns + INSNS_EXPAND_SIZE;
    RTLInsn *new_insns =
        rtl_realloc(unit, unit->insns, sizeof(*unit->insns) * new_insns_size);
    if (UNLIKELY(!new_insns)) {
        log_error(unit->handle, "No memory to expand unit to %u instructions",
                  new_insns_size);
        return false;
    }
    memset(&new_insns[unit->insns_size], 0,
           sizeof(*new_insns) * (new_insns_size - unit->insns_size));
    unit->insns = new_insns;
    unit->insns_size = new_insns_size;

    /* Run back through rtl_add_insn() to handle the rest. */
    return rtl_add_insn(unit, opcode, dest, src1, src2, other);
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_add_insn_with_new_block:  Start a new basic block and then add a new
 * instruction.  Called by rtl_add_insn() when there is no current basic
 * block upon entry.
 *
 * This function is explicitly NOINLINE to avoid having to spill the
 * function parameter registers on the rtl_add_insn() fast path.
 */
static NOINLINE bool rtl_add_insn_with_new_block(
    RTLUnit *unit, RTLOpcode opcode, uint32_t dest, uint32_t src1,
    uint32_t src2, uint64_t other)
{
    if (UNLIKELY(!rtl_block_add(unit))) {
        return false;
    }
    unit->have_block = 1;
    unit->cur_block = unit->num_blocks - 1;
    unit->blocks[unit->cur_block].first_insn = unit->num_insns;

    /* Run back through rtl_add_insn() to handle the rest. */
    return rtl_add_insn(unit, opcode, dest, src1, src2, other);
}

/*-----------------------------------------------------------------------*/

/**
 * snprintf_assert:  Wrapper for snprintf() which ASSERT()s that the
 * written string fits within the supplied buffer.  Helper for
 * rtl_decode_insn() and rtl_describe_block().
 */
static int snprintf_assert(char *buf, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    va_end(args);
    ASSERT((size_t)result < size);
    return result;
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_describe_register:  Generate a string describing the contents of the
 * given RTL register.
 *
 * [Parameters]
 *     reg: Register to describe.
 *     buf: Buffer into which to store description.
 *     bufsize: Size of buffer (should be at least 100).
 */
static void rtl_describe_register(const RTLRegister *reg,
                                  char *buf, int bufsize)
{
    ASSERT(reg != NULL);
    ASSERT(buf != NULL);
    ASSERT(bufsize > 0);

    switch ((RTLRegType)reg->source) {

      case RTLREG_UNDEFINED:
        /* This is only reachable if code incorrectly generates an
         * instruction that uses an undefined register and operand sanity
         * checks are disabled, but we handle it anyway for the sake of
         * completeness. */
        snprintf(buf, bufsize, "(undefined)");
        return;

      case RTLREG_CONSTANT:
        switch ((RTLDataType)reg->type) {
          case RTLTYPE_INT32:
            if (reg->value.int32 >= 0x10000
             && reg->value.int32 < (uint32_t)-0x8000) {
                snprintf(buf, bufsize, "0x%X", reg->value.int32);
            } else {
                snprintf(buf, bufsize, "%d", reg->value.int32);
            }
            return;
          case RTLTYPE_ADDRESS:
            snprintf(buf, bufsize, "0x%"PRIX64, reg->value.address);
            return;
          case RTLTYPE_FLOAT:
          case RTLTYPE_DOUBLE:
          case RTLTYPE_V2_DOUBLE:
            break;  // FIXME: not yet implemented
        }
        ASSERT(!"Invalid constant type");

      case RTLREG_FUNC_ARG:
        snprintf(buf, bufsize, "arg[%u]", reg->arg_index);
        return;

      case RTLREG_MEMORY: {
        const char *type = NULL;
        switch ((RTLDataType)reg->type) {
          case RTLTYPE_INT32:
            type = (reg->memory.size == 0 ? "i32" :
                    reg->memory.size == 2
                        ? (reg->memory.is_signed ? "s16" : "u16")
                        : (reg->memory.is_signed ? "s8" : "u8"));
            break;
          case RTLTYPE_ADDRESS:
            type = "addr";
            break;
          case RTLTYPE_FLOAT:
            type = "f32";
            break;
          case RTLTYPE_DOUBLE:
            type = "f64";
            break;
          case RTLTYPE_V2_DOUBLE:
            type = "f64x2";
            break;
        }
        ASSERT(type);
        snprintf(buf, bufsize, "@%d(r%u).%s%s",
                 reg->memory.offset, reg->memory.addr_reg, type,
                 reg->memory.byterev ? ".br" : "");
        return;
      }

      case RTLREG_ALIAS:
        snprintf(buf, bufsize, "a%u", reg->alias.src);
        return;

      case RTLREG_RESULT:
      case RTLREG_RESULT_NOFOLD: {
        static const char * const operators[] = {
            [RTLOP_SCAST ] = "scast",
            [RTLOP_ZCAST ] = "zcast",
            [RTLOP_ADD   ] = "+",
            [RTLOP_SUB   ] = "-",
            [RTLOP_NEG   ] = "-",
            [RTLOP_MUL   ] = "*",
            [RTLOP_DIVU  ] = "/",
            [RTLOP_DIVS  ] = "/",
            [RTLOP_MODU  ] = "%",
            [RTLOP_MODS  ] = "%",
            [RTLOP_AND   ] = "&",
            [RTLOP_OR    ] = "|",
            [RTLOP_XOR   ] = "^",
            [RTLOP_NOT   ] = "~",
            [RTLOP_SLL   ] = "<<",
            [RTLOP_SRL   ] = ">>",
            [RTLOP_SRA   ] = ">>",
            [RTLOP_ROR   ] = "ror",
            [RTLOP_CLZ   ] = "clz",
            [RTLOP_SLTU  ] = "<",
            [RTLOP_SLTS  ] = "<",
            [RTLOP_SLEU  ] = "<=",
            [RTLOP_SLES  ] = "<=",
            [RTLOP_SEQ   ] = "==",
            [RTLOP_BSWAP ] = "bswap",
        };
        static const bool is_signed[] = {
            [RTLOP_DIVS ] = true,
            [RTLOP_MODS ] = true,
            [RTLOP_SRA  ] = true,
            [RTLOP_SLTS ] = true,
            [RTLOP_SLES ] = true,
        };

        switch (reg->result.opcode) {
          case RTLOP_MOVE:
            snprintf(buf, bufsize, "r%u", reg->result.src1);
            break;
          case RTLOP_SELECT:
            snprintf(buf, bufsize, "r%u ? r%u : r%u", reg->result.cond,
                     reg->result.src1, reg->result.src2);
            break;
          case RTLOP_NEG:
          case RTLOP_NOT:
            snprintf(buf, bufsize, "%sr%u",
                     operators[reg->result.opcode], reg->result.src1);
            break;
          case RTLOP_SCAST:
          case RTLOP_ZCAST:
          case RTLOP_CLZ:
          case RTLOP_BSWAP:
            snprintf(buf, bufsize, "%s(r%u)",
                     operators[reg->result.opcode], reg->result.src1);
            break;
          case RTLOP_ADD:
          case RTLOP_SUB:
          case RTLOP_MUL:
          case RTLOP_DIVU:
          case RTLOP_DIVS:
          case RTLOP_MODU:
          case RTLOP_MODS:
          case RTLOP_AND:
          case RTLOP_OR:
          case RTLOP_XOR:
          case RTLOP_SLL:
          case RTLOP_SRL:
          case RTLOP_SRA:
          case RTLOP_ROR:
          case RTLOP_SLTU:
          case RTLOP_SLTS:
          case RTLOP_SLEU:
          case RTLOP_SLES:
          case RTLOP_SEQ:
            snprintf(buf, bufsize, "%sr%u %s r%u",
                     is_signed[reg->result.opcode] ? "(signed) " : "",
                     reg->result.src1, operators[reg->result.opcode],
                     reg->result.src2);
            break;
          case RTLOP_MULHU:
          case RTLOP_MULHS:
            snprintf(buf, bufsize, "%shi(r%u * r%u)",
                     reg->result.opcode == RTLOP_MULHS ? "(signed) " : "",
                     reg->result.src1, reg->result.src2);
            break;
          case RTLOP_BFEXT:
            snprintf(buf, bufsize, "bfext(r%u, %u, %u)",
                     reg->result.src1, reg->result.start, reg->result.count);
            break;
          case RTLOP_BFINS:
            snprintf(buf, bufsize, "bfins(r%u, r%u, %u, %u)",
                     reg->result.src1, reg->result.src2,
                     reg->result.start, reg->result.count);
            break;
          default:
            ASSERT(!"Invalid result opcode");
            break;
        }  // switch (reg->result.opcode)
        return;
      }  // case RTLREG_RESULT, RTLREG_RESULT_NOFOLD

    }  // switch (reg->source)

    ASSERT(!"Invalid register source");
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_decode_insn:  Decode an RTL instruction into a human-readable string.
 *
 * [Parameters]
 *     unit: RTLUnit containing instruction to decode.
 *     index: Index of instruction to decode.
 *     buf: Buffer into which to store result text.
 *     bufsize: Size of buffer (should be at least 500).
 *     verbose: True to append register descriptions to the instruction.
 */
static void rtl_decode_insn(const RTLUnit *unit, uint32_t index,
                            char *buf, int bufsize, bool verbose)
{
    ASSERT(unit != NULL);
    ASSERT(unit->insns != NULL);
    ASSERT(index < unit->num_insns);
    ASSERT(buf != NULL);

    static const char * const opcode_names[] = {
        [RTLOP_NOP       ] = "NOP",
        [RTLOP_SET_ALIAS ] = "SET_ALIAS",
        [RTLOP_GET_ALIAS ] = "GET_ALIAS",
        [RTLOP_MOVE      ] = "MOVE",
        [RTLOP_SELECT    ] = "SELECT",
        [RTLOP_SCAST     ] = "SCAST",
        [RTLOP_ZCAST     ] = "ZCAST",
        [RTLOP_ADD       ] = "ADD",
        [RTLOP_SUB       ] = "SUB",
        [RTLOP_NEG       ] = "NEG",
        [RTLOP_MUL       ] = "MUL",
        [RTLOP_MULHU     ] = "MULHU",
        [RTLOP_MULHS     ] = "MULHS",
        [RTLOP_DIVU      ] = "DIVU",
        [RTLOP_DIVS      ] = "DIVS",
        [RTLOP_MODU      ] = "MODU",
        [RTLOP_MODS      ] = "MODS",
        [RTLOP_AND       ] = "AND",
        [RTLOP_OR        ] = "OR",
        [RTLOP_XOR       ] = "XOR",
        [RTLOP_NOT       ] = "NOT",
        [RTLOP_SLL       ] = "SLL",
        [RTLOP_SRL       ] = "SRL",
        [RTLOP_SRA       ] = "SRA",
        [RTLOP_ROR       ] = "ROR",
        [RTLOP_CLZ       ] = "CLZ",
        [RTLOP_SLTU      ] = "SLTU",
        [RTLOP_SLTS      ] = "SLTS",
        [RTLOP_SLEU      ] = "SLEU",
        [RTLOP_SLES      ] = "SLES",
        [RTLOP_SEQ       ] = "SEQ",
        [RTLOP_BSWAP     ] = "BSWAP",
        [RTLOP_BFEXT     ] = "BFEXT",
        [RTLOP_BFINS     ] = "BFINS",
        [RTLOP_LOAD_IMM  ] = "LOAD_IMM",
        [RTLOP_LOAD_ARG  ] = "LOAD_ARG",
        [RTLOP_LOAD      ] = "LOAD",
        [RTLOP_LOAD_U8   ] = "LOAD_U8",
        [RTLOP_LOAD_S8   ] = "LOAD_S8",
        [RTLOP_LOAD_U16  ] = "LOAD_U16",
        [RTLOP_LOAD_S16  ] = "LOAD_S16",
        [RTLOP_STORE     ] = "STORE",
        [RTLOP_STORE_I8  ] = "STORE_I8",
        [RTLOP_STORE_I16 ] = "STORE_I16",
        [RTLOP_LOAD_BR   ] = "LOAD_BR",
        [RTLOP_LOAD_U16_BR] = "LOAD_U16_BR",
        [RTLOP_LOAD_S16_BR] = "LOAD_S16_BR",
        [RTLOP_STORE_BR  ] = "STORE_BR",
        [RTLOP_STORE_I16_BR] = "STORE_I16_BR",
        [RTLOP_LABEL     ] = "LABEL",
        [RTLOP_GOTO      ] = "GOTO",
        [RTLOP_GOTO_IF_Z ] = "GOTO_IF_Z",
        [RTLOP_GOTO_IF_NZ] = "GOTO_IF_NZ",
        [RTLOP_RETURN    ] = "RETURN",
        [RTLOP_ILLEGAL   ] = "ILLEGAL",
    };

    char *s = buf;
    char * const top = buf + bufsize;
    char regbuf[100];

    #define APPEND_REG_DESC(regnum)  do { if (verbose) { \
        const unsigned int _regnum = (regnum); \
        rtl_describe_register(&unit->regs[_regnum], regbuf, sizeof(regbuf)); \
        s += snprintf_assert(s, top - s, "           r%u: %s\n", \
                             _regnum, regbuf); \
    } } while (0)

    const RTLInsn * const insn = &unit->insns[index];
    const char * const name = opcode_names[insn->opcode];
    const unsigned int dest = insn->dest;
    const unsigned int src1 = insn->src1;
    const unsigned int src2 = insn->src2;

    s += snprintf_assert(s, top - s, "%5d: ", index);

    switch ((RTLOpcode)insn->opcode) {

      case RTLOP_NOP:
        if (insn->src_imm) {
            s += snprintf_assert(s, top - s, "%-10s 0x%"PRIX64"\n",
                                 name, insn->src_imm);
        } else {
            s += snprintf_assert(s, top - s, "%s\n", name);
        }
        return;

      case RTLOP_SET_ALIAS:
        s += snprintf_assert(s, top - s, "%-10s a%u, r%u\n",
                             name, insn->alias, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_GET_ALIAS:
        s += snprintf_assert(s, top - s, "%-10s r%u, a%u\n",
                             name, dest, insn->alias);
        return;

      case RTLOP_MOVE:
      case RTLOP_SCAST:
      case RTLOP_ZCAST:
      case RTLOP_NEG:
      case RTLOP_NOT:
      case RTLOP_CLZ:
      case RTLOP_BSWAP:
        s += snprintf_assert(s, top - s, "%-10s r%u, r%u\n", name, dest, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_SELECT:
        s += snprintf_assert(s, top - s, "%-10s r%u, r%u, r%u, r%u\n",
                             name, dest, src1, src2, insn->cond);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        APPEND_REG_DESC(insn->cond);
        return;

      case RTLOP_ADD:
      case RTLOP_SUB:
      case RTLOP_MUL:
      case RTLOP_MULHU:
      case RTLOP_MULHS:
      case RTLOP_DIVU:
      case RTLOP_DIVS:
      case RTLOP_MODU:
      case RTLOP_MODS:
      case RTLOP_AND:
      case RTLOP_OR:
      case RTLOP_XOR:
      case RTLOP_SLL:
      case RTLOP_SRL:
      case RTLOP_SRA:
      case RTLOP_ROR:
      case RTLOP_SLTU:
      case RTLOP_SLTS:
      case RTLOP_SLEU:
      case RTLOP_SLES:
      case RTLOP_SEQ:
        s += snprintf_assert(s, top - s, "%-10s r%u, r%u, r%u\n",
                             name, dest, src1, src2);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_BFEXT:
        s += snprintf_assert(s, top - s, "%-10s r%u, r%u, %u, %u\n",
                             name, dest, src1, insn->bitfield.start,
                             insn->bitfield.count);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_BFINS:
        s += snprintf_assert(s, top - s, "%-10s r%u, r%u, r%u, %u, %u\n",
                             name, dest, src1, src2, insn->bitfield.start,
                             insn->bitfield.count);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_LOAD_IMM:
        s += snprintf_assert(s, top - s, "%-10s r%u, ", name, dest);
        ASSERT(dest > 0 && dest < unit->next_reg);
        switch (unit->regs[dest].type) {
          case RTLTYPE_INT32:
            ASSERT(insn->src_imm <= 0xFFFFFFFF);
            if (insn->src_imm >= 0x10000
             && insn->src_imm < (uint32_t)-0x8000) {
                s += snprintf_assert(s, top - s, "0x%"PRIX64"\n",
                                     insn->src_imm);
            } else {
                s += snprintf_assert(s, top - s, "%d\n",
                                     (int32_t)insn->src_imm);
            }
            break;
          case RTLTYPE_ADDRESS:
            s += snprintf_assert(s, top - s, "0x%"PRIX64"\n", insn->src_imm);
            break;
          default:
            ASSERT(!"Invalid type for LOAD_IMM");
        }
        return;

      case RTLOP_LOAD_ARG:
        s += snprintf_assert(s, top - s, "%-10s r%u, %u\n",
                             name, dest, insn->arg_index);
        return;

      case RTLOP_LOAD:
      case RTLOP_LOAD_U8:
      case RTLOP_LOAD_S8:
      case RTLOP_LOAD_U16:
      case RTLOP_LOAD_S16:
      case RTLOP_LOAD_BR:
      case RTLOP_LOAD_U16_BR:
      case RTLOP_LOAD_S16_BR:
        s += snprintf_assert(s, top - s, "%-10s r%u, %d(r%u)\n",
                             name, dest, insn->offset, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_STORE:
      case RTLOP_STORE_I8:
      case RTLOP_STORE_I16:
      case RTLOP_STORE_BR:
      case RTLOP_STORE_I16_BR:
        s += snprintf_assert(s, top - s, "%-10s %d(r%u), r%u\n",
                             name, insn->offset, src1, src2);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_LABEL:
      case RTLOP_GOTO:
        s += snprintf_assert(s, top - s, "%-10s L%u\n", name, insn->label);
        return;

      case RTLOP_GOTO_IF_Z:
      case RTLOP_GOTO_IF_NZ:
        s += snprintf_assert(s, top - s, "%-10s r%u, L%u\n",
                             name, src1, insn->label);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_RETURN:
        if (src1) {
            s += snprintf_assert(s, top - s, "%-10s r%u\n", name, src1);
            APPEND_REG_DESC(src1);
        } else {
            s += snprintf_assert(s, top - s, "%s\n", name);
        }
        return;

      case RTLOP_ILLEGAL:
        s += snprintf_assert(s, top - s, "%s\n", name);
        return;

    }  // switch (insn->opcode)

    ASSERT(!"Invalid opcode");

    #undef APPEND_REG_DESC
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_describe_block:  Generate a string describing the given basic block.
 *
 * [Parameters]
 *     unit: RTLUnit containing basic block to describe.
 *     index: Index of basic block.
 *     buf: Buffer into which to store result text.
 *     bufsize: Size of buffer (should be at least 200).
 */
static void rtl_describe_block(const RTLUnit *unit, int index,
                               char *buf, int bufsize)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(index < unit->num_blocks);
    ASSERT(buf != NULL);

    const RTLBlock * const block = &unit->blocks[index];
    char *s = buf;
    char * const top = buf + bufsize;

    s += snprintf_assert(s, top - s, "Block %4u: ", index);

    if (block->entries[0] < 0) {
        s += snprintf_assert(s, top - s, "<none>");
    } else {
        for (int j = 0; j < lenof(block->entries) && block->entries[j] >= 0;
             j++)
        {
            s += snprintf_assert(s, top - s, "%s%u", j==0 ? "" : ",",
                                 block->entries[j]);
        }
    }

    if (block->first_insn <= block->last_insn) {
        s += snprintf_assert(s, top - s, " --> [%d,%d] --> ",
                             block->first_insn, block->last_insn);
    } else {
        s += snprintf_assert(s, top - s, " --> [empty] --> ");
    }

    if (block->exits[0] < 0) {
        s += snprintf_assert(s, top - s, "<none>");
    } else {
        for (int j = 0; j < lenof(block->exits) && block->exits[j] >= 0; j++) {
            s += snprintf_assert(s, top - s, "%s%u",
                                 j==0 ? "" : ",", block->exits[j]);
        }
    }

    s += snprintf_assert(s, top - s, "\n");
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

RTLUnit *rtl_create_unit(binrec_t *handle)
{
    RTLUnit dummy_unit;  // For calling the handle's malloc() callback.
    dummy_unit.handle = handle;
    RTLUnit *unit = rtl_malloc(&dummy_unit, sizeof(*unit));
    if (!unit) {
        log_error(handle, "No memory for RTLUnit");
        return NULL;
    }

    unit->handle = handle;

    unit->insns = NULL;
    unit->insns_size = INSNS_EXPAND_SIZE;
    unit->num_insns = 0;

    unit->blocks = NULL;
    unit->blocks_size = BLOCKS_EXPAND_SIZE;
    unit->num_blocks = 0;
    unit->have_block = 0;
    unit->cur_block = 0;

    unit->regs = NULL;
    unit->regs_size = REGS_EXPAND_SIZE;
    unit->next_reg = 1;
    unit->first_live_reg = 0;
    unit->last_live_reg = 0;

    unit->alias_types = NULL;
    unit->aliases_size = REGS_EXPAND_SIZE;
    unit->next_alias = 1;

    unit->label_blockmap = NULL;
    unit->labels_size = LABELS_EXPAND_SIZE;
    unit->next_label = 1;

    unit->finalized = 0;

    unit->disassembly = NULL;

    unit->insns = rtl_malloc(unit, sizeof(*unit->insns) * unit->insns_size);
    if (!unit->insns) {
        log_error(handle, "No memory for %u instructions", unit->insns_size);
        goto fail;
    }
    /* Clear instruction buffers so output translators can rely on unused
     * fields in an instruction always being zero. */
    memset(unit->insns, 0, sizeof(*unit->insns) * unit->insns_size);

    unit->blocks = rtl_malloc(unit, sizeof(*unit->blocks) * unit->blocks_size);
    if (!unit->blocks) {
        log_error(handle, "No memory for %u blocks", unit->blocks_size);
        goto fail;
    }

    unit->regs = rtl_malloc(unit, sizeof(*unit->regs) * unit->regs_size);
    if (!unit->regs) {
        log_error(handle, "No memory for %u registers", unit->regs_size);
        goto fail;
    }
    memset(&unit->regs[0], 0, sizeof(*unit->regs));

    unit->alias_types =
        rtl_malloc(unit, sizeof(*unit->alias_types) * unit->aliases_size);
    if (!unit->alias_types) {
        log_error(handle, "No memory for %u alias registers", unit->regs_size);
        goto fail;
    }
    unit->alias_types[0] = 0;

    unit->label_blockmap =
        rtl_malloc(unit, sizeof(*unit->label_blockmap) * unit->labels_size);
    if (!unit->label_blockmap) {
        log_error(handle, "No memory for %u labels", unit->labels_size);
        goto fail;
    }
    unit->label_blockmap[0] = -1;

    return unit;

  fail:
    rtl_free(unit, unit->insns);
    rtl_free(unit, unit->blocks);
    rtl_free(unit, unit->regs);
    rtl_free(unit, unit->alias_types);
    rtl_free(unit, unit->label_blockmap);
    rtl_free(unit, unit);
    return NULL;
}

/*-----------------------------------------------------------------------*/

void rtl_clear_unit(RTLUnit *unit)
{
    ASSERT(unit != NULL);

    unit->num_insns = 0;

    unit->num_blocks = 0;
    unit->have_block = 0;
    unit->cur_block = 0;

    unit->next_reg = 1;
    unit->first_live_reg = 0;
    unit->last_live_reg = 0;

    unit->next_alias = 1;

    unit->next_label = 1;

    unit->finalized = 0;
}

/*-----------------------------------------------------------------------*/

bool rtl_add_insn(RTLUnit *unit, RTLOpcode opcode, uint32_t dest,
                  uint32_t src1, uint32_t src2, uintptr_t other)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);
    ASSERT(opcode >= RTLOP__FIRST && opcode <= RTLOP__LAST);

    /* Extend the instruction array if necessary. */
    if (UNLIKELY(unit->num_insns >= unit->insns_size)) {
        return rtl_add_insn_with_extend(unit, opcode,
                                        dest, src1, src2, other);
    }

    /* Create a new basic block if there's no active one. */
    if (UNLIKELY(!unit->have_block)) {
        return rtl_add_insn_with_new_block(unit, opcode,
                                           dest, src1, src2, other);
    }

    /* Fill in the instruction data. */
    RTLInsn * const insn = &unit->insns[unit->num_insns];
    insn->opcode = opcode;
    if (UNLIKELY(!rtl_insn_make(unit, insn, dest, src1, src2, other))) {
        return false;
    }

    unit->num_insns++;
    return true;
}

/*-----------------------------------------------------------------------*/

uint32_t rtl_alloc_register(RTLUnit *unit, RTLDataType type)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);

    if (UNLIKELY(unit->next_reg >= unit->regs_size)) {
        if (unit->regs_size >= REGS_LIMIT) {
            log_error(unit->handle, "Too many registers in unit (limit %u)",
                      REGS_LIMIT);
            return 0;
        }
        unsigned int new_regs_size;
        if (unit->regs_size > REGS_LIMIT - REGS_EXPAND_SIZE) {
            new_regs_size = REGS_LIMIT;
        } else {
            new_regs_size = unit->next_reg + REGS_EXPAND_SIZE;
        }
        RTLRegister * const new_regs =
            rtl_realloc(unit, unit->regs, sizeof(*unit->regs) * new_regs_size);
        if (UNLIKELY(!new_regs)) {
            log_error(unit->handle, "No memory to expand unit to %u registers",
                      new_regs_size);
            return 0;
        }
        unit->regs = new_regs;
        unit->regs_size = new_regs_size;
    }

    const unsigned int reg_index = unit->next_reg++;
    RTLRegister *reg = &unit->regs[reg_index];
    memset(reg, 0, sizeof(*reg));
    reg->type = type;
    return reg_index;
}

/*-----------------------------------------------------------------------*/

void rtl_make_unique_pointer(RTLUnit *unit, uint32_t reg)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);
    ASSERT(reg != 0 && reg < unit->next_reg);

    unit->regs[reg].unique_pointer = reg;
}

/*-----------------------------------------------------------------------*/

uint32_t rtl_alloc_alias_register(RTLUnit *unit, RTLDataType type)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->alias_types != NULL);

    if (UNLIKELY(unit->next_alias >= unit->aliases_size)) {
        if (unit->aliases_size >= ALIASES_LIMIT) {
            log_error(unit->handle, "Too many alias registers in unit"
                      " (limit %u)", ALIASES_LIMIT);
            return 0;
        }
        unsigned int new_aliases_size;
        if (unit->aliases_size > ALIASES_LIMIT - ALIASES_EXPAND_SIZE) {
            new_aliases_size = ALIASES_LIMIT;
        } else {
            new_aliases_size = unit->next_alias + ALIASES_EXPAND_SIZE;
        }
        RTLDataType * const new_alias_types =
            rtl_realloc(unit, unit->alias_types,
                        sizeof(*unit->alias_types) * new_aliases_size);
        if (UNLIKELY(!new_alias_types)) {
            log_error(unit->handle, "No memory to expand unit to %u alias"
                      " registers", new_aliases_size);
            return 0;
        }
        unit->alias_types = new_alias_types;
        unit->aliases_size = new_aliases_size;
    }

    const unsigned int alias_index = unit->next_alias++;
    unit->alias_types[alias_index] = type;
    return alias_index;
}

/*-----------------------------------------------------------------------*/

uint32_t rtl_alloc_label(RTLUnit *unit)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->label_blockmap != NULL);

    if (UNLIKELY(unit->next_label >= unit->labels_size)) {
        if (unit->labels_size >= LABELS_LIMIT) {
            log_error(unit->handle, "Too many labels in unit (limit %u)",
                      LABELS_LIMIT);
            return 0;
        }
        unsigned int new_labels_size;
        if (unit->labels_size > LABELS_LIMIT - LABELS_EXPAND_SIZE) {
            new_labels_size = LABELS_LIMIT;
        } else {
            new_labels_size = unit->next_label + LABELS_EXPAND_SIZE;
        }
        int16_t * const new_label_blockmap =
            rtl_realloc(unit, unit->label_blockmap,
                        sizeof(*unit->label_blockmap) * new_labels_size);
        if (UNLIKELY(!new_label_blockmap)) {
            log_error(unit->handle, "No memory to expand unit to %u labels",
                      new_labels_size);
            return 0;
        }
        unit->label_blockmap = new_label_blockmap;
        unit->labels_size = new_labels_size;
    }

    const unsigned int label = unit->next_label++;
    unit->label_blockmap[label] = -1;
    return label;
}

/*-----------------------------------------------------------------------*/

bool rtl_finalize_unit(RTLUnit *unit)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);

    /* Terminate the last block (if there is one). */
    if (unit->have_block) {
        unit->blocks[unit->cur_block].last_insn = unit->num_insns - 1;
        unit->have_block = 0;
    }

    /* Add execution graph edges for GOTO instructions. */
    if (UNLIKELY(!add_block_edges(unit))) {
        return false;
    }

    /* Update live ranges for registers used in loops. */
    update_live_ranges(unit);

    unit->finalized = 1;
    return true;
}

/*-----------------------------------------------------------------------*/

bool rtl_optimize_unit(RTLUnit *unit, uint32_t flags)
{
    ASSERT(unit != NULL);
    ASSERT(unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);

#if 0  // FIXME: not yet implemented
    if (flags & BINREC_OPT_FOLD_CONSTANTS) {
        if (UNLIKELY(!rtl_opt_fold_constants(unit))) {
            log_error(unit->handle, "Constant folding failed");
            return false;
        }
    }

    if (flags & BINREC_OPT_DECONDITION) {
        if (UNLIKELY(!rtl_opt_decondition(unit))) {
            log_error(unit->handle, "Deconditioning failed");
            return false;
        }
    }

    if (flags & BINREC_OPT_DROP_DEAD_BLOCKS) {
        if (UNLIKELY(!rtl_opt_drop_dead_blocks(unit))) {
            log_error(unit->handle, "Dead block dropping failed");
            return false;
        }
    }
#endif

    return true;
}

/*-----------------------------------------------------------------------*/

const char *rtl_disassemble_unit(RTLUnit *unit, bool verbose)
{
    ASSERT(unit != NULL);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);

    rtl_free(unit, unit->disassembly);
    unit->disassembly = NULL;

    char *buf = NULL;
    int buflen = 0;
    int bufsize = 0;

    for (uint32_t index = 0; index < unit->num_insns; index++) {
        char text[500];
        rtl_decode_insn(unit, index, text, sizeof(text), verbose);
        const int text_len = strlen(text);
        /* We leave an extra byte here for the blank line we insert between
         * the disassembly and block list. */
        if (buflen + text_len + 2 > bufsize) {
            const int newsize = buflen + text_len + 2 + 10000;
            char *newbuf = rtl_realloc(unit, buf, newsize);
            if (UNLIKELY(!newbuf)) {
                log_error(unit->handle, "Out of memory disassembling unit");
                rtl_free(unit, buf);
                return NULL;
            }
            buf = newbuf;
            bufsize = newsize;
        }
        buflen += snprintf(&buf[buflen], (bufsize-1) - buflen, "%s", text);
        ASSERT(buflen + 1 < bufsize);
    }

    buf[buflen++] = '\n';
    ASSERT(buflen < bufsize);
    buf[buflen] = '\0';

    for (unsigned int index = 0; index < unit->num_blocks; index++) {
        char text[200];
        rtl_describe_block(unit, index, text, sizeof(text));
        const int text_len = strlen(text);
        if (buflen + text_len + 1 > bufsize) {
            const int newsize = buflen + text_len + 1 + 10000;
            char *newbuf = rtl_realloc(unit, buf, newsize);
            if (UNLIKELY(!newbuf)) {
                log_error(unit->handle, "Out of memory disassembling unit");
                rtl_free(unit, buf);
                return NULL;
            }
            buf = newbuf;
            bufsize = newsize;
        }
        buflen += snprintf(&buf[buflen], bufsize - buflen, "%s", text);
        ASSERT(buflen + 1 <= bufsize);
    }

    unit->disassembly = buf;
    return buf;
}

/*-----------------------------------------------------------------------*/

void rtl_destroy_unit(RTLUnit *unit)
{
    if (unit) {
        rtl_free(unit, unit->insns);
        rtl_free(unit, unit->blocks);
        rtl_free(unit, unit->regs);
        rtl_free(unit, unit->alias_types);
        rtl_free(unit, unit->label_blockmap);
        rtl_free(unit, unit->disassembly);
        rtl_free(unit, unit);
    }
}

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
