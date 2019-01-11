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
#include "src/rtl.h"
#include "src/rtl-internal.h"

#include <inttypes.h>
#include <math.h>
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

    for (unsigned int block_index = 0; block_index < unit->num_blocks;
         block_index++)
    {
        RTLBlock * const block = &unit->blocks[block_index];
        if (block->first_insn <= block->last_insn) {
            const RTLInsn * const insn = &unit->insns[block->last_insn];
            if (insn->opcode == RTLOP_GOTO
             || insn->opcode == RTLOP_GOTO_IF_Z
             || insn->opcode == RTLOP_GOTO_IF_NZ) {
                const int label = insn->label;
                if (UNLIKELY(unit->label_blockmap[label] < 0)) {
                    log_error(unit->handle, "GOTO to undefined label L%d at"
                              " %d", label, block->last_insn);
                    return false;
                } else if (UNLIKELY(!rtl_block_add_edge(unit, block_index, unit->label_blockmap[label]))) {
                    static const char * const opcode_names[] = {
                        [RTLOP_GOTO       - RTLOP_GOTO] = "GOTO",
                        [RTLOP_GOTO_IF_Z  - RTLOP_GOTO] = "GOTO_IF_Z",
                        [RTLOP_GOTO_IF_NZ - RTLOP_GOTO] = "GOTO_IF_NZ",
                    };
                    log_ice(unit->handle, "Failed to add edge %d->%d for"
                            " %s L%d at %d",
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
 * rtl_add_insn_with_extend:  Extend the instruction array and then add a
 * new instruction.  Called by rtl_add_insn() when the instruction array is
 * full upon entry.
 *
 * This function is explicitly NOINLINE to avoid having to spill the
 * function parameter registers on the rtl_add_insn() fast path.
 */
static NOINLINE bool rtl_add_insn_with_extend(
    RTLUnit *unit, RTLOpcode opcode,
    int dest, int src1, int src2, uint64_t other)
{
    uint32_t new_insns_size = unit->num_insns + INSNS_EXPAND_SIZE;
    RTLInsn *new_insns =
        rtl_realloc(unit, unit->insns, sizeof(*unit->insns) * new_insns_size);
    if (UNLIKELY(!new_insns)) {
        log_error(unit->handle, "No memory to expand unit to %u instructions",
                  new_insns_size);
        unit->error = true;
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
    RTLUnit *unit, RTLOpcode opcode,
    int dest, int src1, int src2, uint64_t other)
{
    if (UNLIKELY(!rtl_block_add(unit))) {
        unit->error = true;
        return false;
    }
    unit->have_block = true;
    unit->cur_block = unit->num_blocks - 1;
    unit->blocks[unit->cur_block].first_insn = unit->num_insns;

    /* Run back through rtl_add_insn() to handle the rest. */
    return rtl_add_insn(unit, opcode, dest, src1, src2, other);
}

/*-----------------------------------------------------------------------*/

/**
 * verify_reg:  Verify that the given register is valid and is initialized
 * by its birth instruction.
 *
 * [Parameters]
 *     unit: RTL unit to operate on.
 *     reg_index: Index of register to verify.
 *     address: Guest code address associated with unit.
 *     insn_index: Index of instruction referencing register.
 * [Return value]
 *     True if register is properly initialized, false if not.
 */
static bool verify_reg(RTLUnit *unit, int reg_index,
                       uint32_t address, int insn_index)
{
    if (reg_index < 1 || reg_index >= unit->regs_size) {
        log_error(unit->handle, "0x%X/%d: Register r%d invalid",
                  address, insn_index, reg_index);
        return false;
    }
    const RTLRegister *reg = &unit->regs[reg_index];
    if (reg->birth < 0 || (uint32_t)reg->birth >= unit->num_insns) {
        log_error(unit->handle,
                  "0x%X/%d: Register r%d has invalid birth insn %d",
                  address, insn_index, reg_index, reg->birth);
        return false;
    }
    if (unit->insns[reg->birth].dest != reg_index
     || unit->insns[reg->birth].opcode == RTLOP_NOP) {
        log_error(unit->handle, "0x%X/%d: Register r%d is not initialized",
                  address, insn_index, reg_index);
        return false;
    }
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * fcmp_name:  Return the name of the given floating-point comparison
 * (RTLFCMP_*).
 */
static inline CONST_FUNCTION const char *fcmp_name(RTLFloatCompare fcmp)
{
    switch (fcmp) {
        case RTLFCMP_LT: return "LT";
        case RTLFCMP_LE: return "LE";
        case RTLFCMP_GT: return "GT";
        case RTLFCMP_GE: return "GE";
        case RTLFCMP_EQ: return "EQ";
        case RTLFCMP_UN: return "UN";
    }
    return "???";
}

/*-----------------------------------------------------------------------*/

/**
 * fexc_name:  Return the name of the given floating-point exception
 * (RTLFEXC_*).
 */
static inline CONST_FUNCTION const char *fexc_name(RTLFloatException fexc)
{
    switch (fexc) {
        case RTLFEXC_ANY:         return "ANY";
        case RTLFEXC_INEXACT:     return "INEXACT";
        case RTLFEXC_INVALID:     return "INVALID";
        case RTLFEXC_OVERFLOW:    return "OVERFLOW";
        case RTLFEXC_UNDERFLOW:   return "UNDERFLOW";
        case RTLFEXC_ZERO_DIVIDE: return "ZERO_DIVIDE";
    }
    return "???";
}

/*-----------------------------------------------------------------------*/

/**
 * fround_name:  Return the name of the given floating-point rounding mode
 * (RTLFROUND_*).
 */
static inline CONST_FUNCTION const char *fround_name(RTLFloatRoundingMode mode)
{
    switch (mode) {
        case RTLFROUND_NEAREST: return "NEAREST";
        case RTLFROUND_TRUNC:   return "TRUNC";
        case RTLFROUND_FLOOR:   return "FLOOR";
        case RTLFROUND_CEIL:    return "CEIL";
    }
    return "???";
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
      case RTLREG_CONSTANT_NOFOLD:
        switch ((RTLDataType)reg->type) {
          case RTLTYPE_FLOAT32:
            format_float(buf, bufsize, reg->value.f32);
            break;
          case RTLTYPE_FLOAT64:
            format_double(buf, bufsize, reg->value.f64);
            break;
          default:
            ASSERT(rtl_register_is_int(reg));
            format_int(buf, bufsize, reg->type, reg->value.i64);
            break;
        }
        return;

      case RTLREG_FUNC_ARG:
        snprintf(buf, bufsize, "arg[%d]", reg->arg_index);
        return;

      case RTLREG_MEMORY: {
        const char *type;
        if (reg->type == RTLTYPE_INT32 && reg->memory.size != 0) {
            type = (reg->memory.size == 2
                    ? (reg->memory.is_signed ? "s16" : "u16")
                    : (reg->memory.is_signed ? "s8" : "u8"));
        } else {
            type = rtl_type_suffix(reg->type);
        }
        snprintf(buf, bufsize, "@%d(r%d).%s%s",
                 reg->memory.offset, reg->memory.base, type,
                 reg->memory.byterev ? ".br" : "");
        return;
      }

      case RTLREG_ALIAS:
        snprintf(buf, bufsize, "a%d", reg->alias.src);
        return;

      case RTLREG_RESULT:
      case RTLREG_RESULT_NOFOLD: {
        static const char * const operators[RTLOP__LAST + 1] = {
            [RTLOP_SCAST  ] = "scast",
            [RTLOP_ZCAST  ] = "zcast",
            [RTLOP_SEXT8  ] = "sext8",
            [RTLOP_SEXT16 ] = "sext16",
            [RTLOP_ADD    ] = "+",
            [RTLOP_ADDI   ] = "+",
            [RTLOP_FADD   ] = "+",
            [RTLOP_SUB    ] = "-",
            [RTLOP_FSUB   ] = "-",
            [RTLOP_NEG    ] = "-",
            [RTLOP_MUL    ] = "*",
            [RTLOP_MULI   ] = "*",
            [RTLOP_FMUL   ] = "*",
            [RTLOP_DIVU   ] = "/",
            [RTLOP_DIVS   ] = "/",
            [RTLOP_FDIV   ] = "/",
            [RTLOP_MODU   ] = "%",
            [RTLOP_MODS   ] = "%",
            [RTLOP_AND    ] = "&",
            [RTLOP_ANDI   ] = "&",
            [RTLOP_OR     ] = "|",
            [RTLOP_ORI    ] = "|",
            [RTLOP_XOR    ] = "^",
            [RTLOP_XORI   ] = "^",
            [RTLOP_NOT    ] = "~",
            [RTLOP_SLL    ] = "<<",
            [RTLOP_SLLI   ] = "<<",
            [RTLOP_SRL    ] = ">>",
            [RTLOP_SRLI   ] = ">>",
            [RTLOP_SRA    ] = ">>",
            [RTLOP_SRAI   ] = ">>",
            [RTLOP_ROL    ] = "rol",
            [RTLOP_ROR    ] = "ror",
            [RTLOP_RORI   ] = "ror",
            [RTLOP_CLZ    ] = "clz",
            [RTLOP_BSWAP  ] = "bswap",
            [RTLOP_SEQ    ] = "==",
            [RTLOP_SEQI   ] = "==",
            [RTLOP_SLTU   ] = "<",
            [RTLOP_SLTUI  ] = "<",
            [RTLOP_SLTS   ] = "<",
            [RTLOP_SLTSI  ] = "<",
            [RTLOP_SGTU   ] = ">",
            [RTLOP_SGTUI  ] = ">",
            [RTLOP_SGTS   ] = ">",
            [RTLOP_SGTSI  ] = ">",
            [RTLOP_BITCAST] = "bitcast",
            [RTLOP_FCVT   ] = "fcvt",
            [RTLOP_FZCAST ] = "fzcast",
            [RTLOP_FSCAST ] = "fscast",
            [RTLOP_FROUNDI] = "froundi",
            [RTLOP_FTRUNCI] = "ftrunci",
            [RTLOP_FNEG   ] = "-",
            [RTLOP_FABS   ] = "abs",
            [RTLOP_FNABS  ] = "-abs",
            [RTLOP_FSQRT  ] = "sqrt",
            [RTLOP_FCMP   ] = "fcmp",
            [RTLOP_FCLEAREXC] = "fclearexc",
            [RTLOP_VFCVT  ] = "vfcvt",
            [RTLOP_VFCMP  ] = "vfcmp",
        };
        static const bool is_signed[RTLOP__LAST + 1] = {
            [RTLOP_DIVS ] = true,
            [RTLOP_MODS ] = true,
            [RTLOP_SRA  ] = true,
            [RTLOP_SRAI ] = true,
            [RTLOP_SLTS ] = true,
            [RTLOP_SLTSI] = true,
            [RTLOP_SGTS ] = true,
            [RTLOP_SGTSI] = true,
        };

        switch ((RTLOpcode)reg->result.opcode) {
          case RTLOP_MOVE:
            snprintf(buf, bufsize, "r%d", reg->result.src1);
            break;
          case RTLOP_SELECT:
            snprintf(buf, bufsize, "r%d ? r%d : r%d", reg->result.src3,
                     reg->result.src1, reg->result.src2);
            break;
          case RTLOP_NEG:
          case RTLOP_NOT:
            snprintf(buf, bufsize, "%sr%d",
                     operators[reg->result.opcode], reg->result.src1);
            break;
          case RTLOP_SCAST:
          case RTLOP_ZCAST:
          case RTLOP_SEXT8:
          case RTLOP_SEXT16:
          case RTLOP_CLZ:
          case RTLOP_BSWAP:
          case RTLOP_BITCAST:
          case RTLOP_FCVT:
          case RTLOP_FZCAST:
          case RTLOP_FSCAST:
          case RTLOP_FROUNDI:
          case RTLOP_FTRUNCI:
          case RTLOP_FNEG:
          case RTLOP_FABS:
          case RTLOP_FNABS:
          case RTLOP_FSQRT:
          case RTLOP_FCLEAREXC:
          case RTLOP_VFCVT:
            snprintf(buf, bufsize, "%s(r%d)",
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
          case RTLOP_ROL:
          case RTLOP_ROR:
          case RTLOP_SEQ:
          case RTLOP_SLTU:
          case RTLOP_SLTS:
          case RTLOP_SGTU:
          case RTLOP_SGTS:
          case RTLOP_FADD:
          case RTLOP_FSUB:
          case RTLOP_FMUL:
          case RTLOP_FDIV:
            snprintf(buf, bufsize, "%sr%d %s r%d",
                     is_signed[reg->result.opcode] ? "(signed) " : "",
                     reg->result.src1, operators[reg->result.opcode],
                     reg->result.src2);
            break;
          case RTLOP_ADDI:
          case RTLOP_MULI:
          case RTLOP_ANDI:
          case RTLOP_ORI:
          case RTLOP_XORI:
          case RTLOP_SLLI:
          case RTLOP_SRLI:
          case RTLOP_SRAI:
          case RTLOP_RORI:
          case RTLOP_SEQI:
          case RTLOP_SLTUI:
          case RTLOP_SLTSI:
          case RTLOP_SGTUI:
          case RTLOP_SGTSI:
            snprintf(buf, bufsize, "%sr%d %s %d",
                     is_signed[reg->result.opcode] ? "(signed) " : "",
                     reg->result.src1, operators[reg->result.opcode],
                     reg->result.src_imm);
            break;
          case RTLOP_MULHU:
          case RTLOP_MULHS:
            snprintf(buf, bufsize, "%shi(r%d * r%d)",
                     reg->result.opcode == RTLOP_MULHS ? "(signed) " : "",
                     reg->result.src1, reg->result.src2);
            break;
          case RTLOP_BFEXT:
            snprintf(buf, bufsize, "bfext(r%d, %d, %d)",
                     reg->result.src1, reg->result.start, reg->result.count);
            break;
          case RTLOP_BFINS:
            snprintf(buf, bufsize, "bfins(r%d, r%d, %d, %d)",
                     reg->result.src1, reg->result.src2,
                     reg->result.start, reg->result.count);
            break;
          case RTLOP_FCMP:
          case RTLOP_VFCMP:
            snprintf(buf, bufsize, "%s(r%d, r%d, %s%s%s)",
                     operators[reg->result.opcode],
                     reg->result.src1, reg->result.src2,
                     reg->result.fcmp & 16 ? "O" : "",
                     reg->result.fcmp & 8 ? "N" : "",
                     fcmp_name(reg->result.fcmp & 7));
            break;
          case RTLOP_FMADD:
          case RTLOP_FMSUB:
          case RTLOP_FNMADD:
          case RTLOP_FNMSUB:
            snprintf(buf, bufsize, "%sfma(r%d, r%d, %sr%d)",
                     (reg->result.opcode == RTLOP_FNMADD
                      || reg->result.opcode == RTLOP_FNMSUB ? "-" : ""),
                     reg->result.src1, reg->result.src2,
                     (reg->result.opcode == RTLOP_FMSUB
                      || reg->result.opcode == RTLOP_FNMSUB ? "-" : ""),
                     reg->result.src3);
            break;
          case RTLOP_FGETSTATE:
            snprintf(buf, bufsize, "fgetstate()");
            break;
          case RTLOP_FTESTEXC:
            snprintf(buf, bufsize, "ftestexc(r%d, %s)",
                     reg->result.src1, fexc_name(reg->result.src_imm));
            break;
          case RTLOP_FSETROUND:
            snprintf(buf, bufsize, "fsetround(r%d, %s)",
                     reg->result.src1, fround_name(reg->result.src_imm));
            break;
          case RTLOP_FCOPYROUND:
            snprintf(buf, bufsize, "fcopyround(r%d, r%d)",
                     reg->result.src1, reg->result.src2);
            break;
          case RTLOP_VBUILD2:
            snprintf(buf, bufsize, "{r%d, r%d}",
                     reg->result.src1, reg->result.src2);
            break;
          case RTLOP_VBROADCAST:
            snprintf(buf, bufsize, "vbroadcast(r%d)", reg->result.src1);
            break;
          case RTLOP_VEXTRACT:
            snprintf(buf, bufsize, "r%d[%d]",
                     reg->result.src1, reg->result.elem);
            break;
          case RTLOP_VINSERT:
            snprintf(buf, bufsize, "vinsert(r%d, r%d, %d)",
                     reg->result.src1, reg->result.src2, reg->result.elem);
            break;
          case RTLOP_ATOMIC_INC:
            snprintf(buf, bufsize, "atomic_inc((r%d).%s)",
                     reg->result.src1, rtl_type_suffix(reg->type));
            break;
          case RTLOP_CMPXCHG:
            snprintf(buf, bufsize, "cmpxchg((r%d).%s, r%d, r%d)",
                     reg->result.src1, rtl_type_suffix(reg->type),
                     reg->result.src2, reg->result.src3);
            break;
          case RTLOP_CALL:
          case RTLOP_CALL_TRANSPARENT:
            snprintf(buf, bufsize, "call(...)");
            break;
          default:
            snprintf(buf, bufsize, "<invalid result opcode %u>",
                     reg->result.opcode);
            break;
        }  // switch (reg->result.opcode)
        return;
      }  // case RTLREG_RESULT, RTLREG_RESULT_NOFOLD

    }  // switch (reg->source)

    snprintf(buf, bufsize, "<invalid register source %u>", reg->source);
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
        [RTLOP_SEXT8     ] = "SEXT8",
        [RTLOP_SEXT16    ] = "SEXT16",
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
        [RTLOP_ROL       ] = "ROL",
        [RTLOP_ROR       ] = "ROR",
        [RTLOP_CLZ       ] = "CLZ",
        [RTLOP_BSWAP     ] = "BSWAP",
        [RTLOP_SEQ       ] = "SEQ",
        [RTLOP_SLTU      ] = "SLTU",
        [RTLOP_SLTS      ] = "SLTS",
        [RTLOP_SGTU      ] = "SGTU",
        [RTLOP_SGTS      ] = "SGTS",
        [RTLOP_BFEXT     ] = "BFEXT",
        [RTLOP_BFINS     ] = "BFINS",
        [RTLOP_ADDI      ] = "ADDI",
        [RTLOP_MULI      ] = "MULI",
        [RTLOP_ANDI      ] = "ANDI",
        [RTLOP_ORI       ] = "ORI",
        [RTLOP_XORI      ] = "XORI",
        [RTLOP_SLLI      ] = "SLLI",
        [RTLOP_SRLI      ] = "SRLI",
        [RTLOP_SRAI      ] = "SRAI",
        [RTLOP_RORI      ] = "RORI",
        [RTLOP_SEQI      ] = "SEQI",
        [RTLOP_SLTUI     ] = "SLTUI",
        [RTLOP_SLTSI     ] = "SLTSI",
        [RTLOP_SGTUI     ] = "SGTUI",
        [RTLOP_SGTSI     ] = "SGTSI",
        [RTLOP_BITCAST   ] = "BITCAST",
        [RTLOP_FCVT      ] = "FCVT",
        [RTLOP_FZCAST    ] = "FZCAST",
        [RTLOP_FSCAST    ] = "FSCAST",
        [RTLOP_FROUNDI   ] = "FROUNDI",
        [RTLOP_FTRUNCI   ] = "FTRUNCI",
        [RTLOP_FNEG      ] = "FNEG",
        [RTLOP_FABS      ] = "FABS",
        [RTLOP_FNABS     ] = "FNABS",
        [RTLOP_FADD      ] = "FADD",
        [RTLOP_FSUB      ] = "FSUB",
        [RTLOP_FMUL      ] = "FMUL",
        [RTLOP_FDIV      ] = "FDIV",
        [RTLOP_FSQRT     ] = "FSQRT",
        [RTLOP_FCMP      ] = "FCMP",
        [RTLOP_FMADD     ] = "FMADD",
        [RTLOP_FMSUB     ] = "FMSUB",
        [RTLOP_FNMADD    ] = "FNMADD",
        [RTLOP_FNMSUB    ] = "FNMSUB",
        [RTLOP_FGETSTATE ] = "FGETSTATE",
        [RTLOP_FSETSTATE ] = "FSETSTATE",
        [RTLOP_FTESTEXC  ] = "FTESTEXC",
        [RTLOP_FCLEAREXC ] = "FCLEAREXC",
        [RTLOP_FSETROUND ] = "FSETROUND",
        [RTLOP_FCOPYROUND] = "FCOPYROUND",
        [RTLOP_VBUILD2   ] = "VBUILD2",
        [RTLOP_VBROADCAST] = "VBROADCAST",
        [RTLOP_VEXTRACT  ] = "VEXTRACT",
        [RTLOP_VINSERT   ] = "VINSERT",
        [RTLOP_VFCVT     ] = "VFCVT",
        [RTLOP_VFCMP     ] = "VFCMP",
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
        [RTLOP_ATOMIC_INC] = "ATOMIC_INC",
        [RTLOP_CMPXCHG   ] = "CMPXCHG",
        [RTLOP_LABEL     ] = "LABEL",
        [RTLOP_GOTO      ] = "GOTO",
        [RTLOP_GOTO_IF_Z ] = "GOTO_IF_Z",
        [RTLOP_GOTO_IF_NZ] = "GOTO_IF_NZ",
        [RTLOP_CALL      ] = "CALL",
        [RTLOP_CALL_TRANSPARENT] = "CALL_TRANSPARENT",
        [RTLOP_RETURN    ] = "RETURN",
        [RTLOP_CHAIN     ] = "CHAIN",
        [RTLOP_CHAIN_RESOLVE] = "CHAIN_RESOLVE",
        [RTLOP_ILLEGAL   ] = "ILLEGAL",
    };

    char *s = buf;
    char * const top = buf + bufsize;
    char regbuf[100];

    #define APPEND_REG_DESC(regnum)  do { if (verbose) { \
        const int _regnum = (regnum); \
        rtl_describe_register(&unit->regs[_regnum], regbuf, sizeof(regbuf)); \
        s += snprintf_assert(s, top - s, "           r%d: %s\n", \
                             _regnum, regbuf); \
    } } while (0)

    const RTLInsn * const insn = &unit->insns[index];
    const char * const name = opcode_names[insn->opcode];
    const int dest = insn->dest;
    const int src1 = insn->src1;
    const int src2 = insn->src2;

    s += snprintf_assert(s, top - s, "%5d: ", index);

    switch ((RTLOpcode)insn->opcode) {

      case RTLOP_ILLEGAL:
        s += snprintf_assert(s, top - s, "%s\n", name);
        return;

      case RTLOP_NOP:
        if (insn->dest || insn->src1 || insn->src2 || insn->src_imm) {
            s += snprintf_assert(s, top - s, "%-10s ", name);
            if (insn->dest) {
                s += snprintf_assert(s, top - s, "r%d", insn->dest);
            } else {
                s += snprintf_assert(s, top - s, "-");
            }
            if (insn->src1) {
                s += snprintf_assert(s, top - s, ", r%d", insn->src1);
            }
            if (insn->src2) {
                if (!insn->src1) {
                    s += snprintf_assert(s, top - s, ", <missing operand>");
                }
                s += snprintf_assert(s, top - s, ", r%d", insn->src2);
            }
            if (insn->src_imm) {
                s += snprintf_assert(s, top - s, ", 0x%"PRIX64, insn->src_imm);
            }
            s += snprintf_assert(s, top - s, "\n");
        } else {
            s += snprintf_assert(s, top - s, "%s\n", name);
        }
        return;

      case RTLOP_SET_ALIAS:
        s += snprintf_assert(s, top - s, "%-10s a%d, r%d\n",
                             name, insn->alias, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_GET_ALIAS:
        s += snprintf_assert(s, top - s, "%-10s r%d, a%d\n",
                             name, dest, insn->alias);
        return;

      case RTLOP_MOVE:
      case RTLOP_SCAST:
      case RTLOP_ZCAST:
      case RTLOP_SEXT8:
      case RTLOP_SEXT16:
      case RTLOP_NEG:
      case RTLOP_NOT:
      case RTLOP_CLZ:
      case RTLOP_BSWAP:
      case RTLOP_BITCAST:
      case RTLOP_FCVT:
      case RTLOP_FZCAST:
      case RTLOP_FSCAST:
      case RTLOP_FROUNDI:
      case RTLOP_FTRUNCI:
      case RTLOP_FNEG:
      case RTLOP_FABS:
      case RTLOP_FNABS:
      case RTLOP_FSQRT:
      case RTLOP_FCLEAREXC:
      case RTLOP_VBROADCAST:
      case RTLOP_VFCVT:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d\n", name, dest, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_SELECT:
      case RTLOP_FMADD:
      case RTLOP_FMSUB:
      case RTLOP_FNMADD:
      case RTLOP_FNMSUB:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, r%d, r%d\n",
                             name, dest, src1, src2, insn->src3);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        APPEND_REG_DESC(insn->src3);
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
      case RTLOP_ROL:
      case RTLOP_ROR:
      case RTLOP_SEQ:
      case RTLOP_SLTU:
      case RTLOP_SLTS:
      case RTLOP_SGTU:
      case RTLOP_SGTS:
      case RTLOP_FADD:
      case RTLOP_FSUB:
      case RTLOP_FMUL:
      case RTLOP_FDIV:
      case RTLOP_FCOPYROUND:
      case RTLOP_VBUILD2:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, r%d\n",
                             name, dest, src1, src2);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_BFEXT:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, %d, %d\n",
                             name, dest, src1, insn->bitfield.start,
                             insn->bitfield.count);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_BFINS:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, r%d, %d, %d\n",
                             name, dest, src1, src2, insn->bitfield.start,
                             insn->bitfield.count);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_ADDI:
      case RTLOP_MULI:
      case RTLOP_ANDI:
      case RTLOP_ORI:
      case RTLOP_XORI:
      case RTLOP_SLLI:
      case RTLOP_SRLI:
      case RTLOP_SRAI:
      case RTLOP_RORI:
      case RTLOP_SEQI:
      case RTLOP_SLTUI:
      case RTLOP_SLTSI:
      case RTLOP_SGTUI:
      case RTLOP_SGTSI:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, %"PRId64"\n",
                             name, dest, src1, insn->src_imm);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_FCMP:
      case RTLOP_VFCMP:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, r%d, %s%s%s\n",
                             name, dest, src1, src2,
                             insn->fcmp & 16 ? "O" : "",
                             insn->fcmp & 8 ? "N" : "",
                             fcmp_name(insn->fcmp & 7));
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_FGETSTATE:
        s += snprintf_assert(s, top - s, "%-10s r%d\n", name, dest);
        return;

      case RTLOP_FSETSTATE:
        s += snprintf_assert(s, top - s, "%-10s r%d\n", name, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_FTESTEXC:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, %s\n",
                             name, dest, src1, fexc_name(insn->src_imm));
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_FSETROUND:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, %s\n",
                             name, dest, src1, fround_name(insn->src_imm));
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_VEXTRACT:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, %d\n",
                             name, dest, src1, insn->elem);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_VINSERT:
        s += snprintf_assert(s, top - s, "%-10s r%d, r%d, r%d, %d\n",
                             name, dest, src1, src2, insn->elem);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_LOAD_IMM:
        s += snprintf_assert(s, top - s, "%-10s r%d, ", name, dest);
        ASSERT(dest > 0 && dest < unit->next_reg);
        switch (unit->regs[dest].type) {
          case RTLTYPE_INT32:
            ASSERT(insn->src_imm <= 0xFFFFFFFF);
            /* fall through */
          default:
            ASSERT(rtl_register_is_int(&unit->regs[dest]));
            s += format_int(s, top - s, unit->regs[dest].type, insn->src_imm);
            break;
          case RTLTYPE_FLOAT32:
            s += format_float(s, top - s,
                              bits_to_float((uint32_t)insn->src_imm));
            break;
          case RTLTYPE_FLOAT64:
            s += format_double(s, top - s, bits_to_double(insn->src_imm));
            break;
        }
        s += snprintf_assert(s, top - s, "\n");
        return;

      case RTLOP_LOAD_ARG:
        s += snprintf_assert(s, top - s, "%-10s r%d, %d\n",
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
        s += snprintf_assert(s, top - s, "%-10s r%d, %d(r%d)\n",
                             name, dest, insn->offset, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_STORE:
      case RTLOP_STORE_I8:
      case RTLOP_STORE_I16:
      case RTLOP_STORE_BR:
      case RTLOP_STORE_I16_BR:
        s += snprintf_assert(s, top - s, "%-10s %d(r%d), r%d\n",
                             name, insn->offset, src1, src2);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        return;

      case RTLOP_ATOMIC_INC:
        s += snprintf_assert(s, top - s, "%-10s r%d, (r%d)\n",
                             name, dest, src1);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_CMPXCHG:
        s += snprintf_assert(s, top - s, "%-10s r%d, (r%d), r%d, r%d\n",
                             name, dest, src1, src2, insn->src3);
        APPEND_REG_DESC(src1);
        APPEND_REG_DESC(src2);
        APPEND_REG_DESC(insn->src3);
        return;

      case RTLOP_LABEL:
      case RTLOP_GOTO:
        s += snprintf_assert(s, top - s, "%-10s L%d\n", name, insn->label);
        return;

      case RTLOP_GOTO_IF_Z:
      case RTLOP_GOTO_IF_NZ:
        s += snprintf_assert(s, top - s, "%-10s r%d, L%d\n",
                             name, src1, insn->label);
        APPEND_REG_DESC(src1);
        return;

      case RTLOP_CALL:
      case RTLOP_CALL_TRANSPARENT:
        s += snprintf_assert(s, top - s, "%-10s ", name);
        if (dest) {
            s += snprintf_assert(s, top - s, "r%d, ", dest);
        }
        s += snprintf_assert(s, top - s, "@r%d", src1);
        if (src2) {
            s += snprintf_assert(s, top - s, ", r%d", src2);
            if (insn->src3) {
                s += snprintf_assert(s, top - s, ", r%d", insn->src3);
            }
        }
        s += snprintf_assert(s, top - s, "\n");
        APPEND_REG_DESC(src1);
        if (src2) {
            APPEND_REG_DESC(src2);
            if (insn->src3) {
                APPEND_REG_DESC(insn->src3);
            }
        }
        return;

      case RTLOP_RETURN:
        if (src1) {
            s += snprintf_assert(s, top - s, "%-10s r%d\n", name, src1);
            APPEND_REG_DESC(src1);
        } else {
            s += snprintf_assert(s, top - s, "%s\n", name);
        }
        return;

      case RTLOP_CHAIN:
        if (src1) {
            s += snprintf_assert(s, top - s, "%-10s r%d", name, src1);
            if (src2) {
                s += snprintf_assert(s, top - s, ", r%d", src2);
            }
        } else {
            s += snprintf_assert(s, top - s, "%s", name);
        }
        s += snprintf_assert(s, top - s, "\n");
        if (src1) {
            APPEND_REG_DESC(src1);
            if (src2) {
                APPEND_REG_DESC(src2);
            }
        }
        return;

      case RTLOP_CHAIN_RESOLVE:
        s += snprintf_assert(s, top - s, "%-10s @%d, r%d\n",
                             name, (int)insn->src_imm, src1);
        APPEND_REG_DESC(src1);
        return;

    }  // switch (insn->opcode)

    s += snprintf_assert(s, top - s, "<invalid opcode %u>\n", insn->opcode);

    #undef APPEND_REG_DESC
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_describe_alias:  Generate a string describing the given alias register.
 *
 * [Parameters]
 *     unit: RTLUnit containing basic block to describe.
 *     index: Alias register number.
 *     buf: Buffer into which to store result text.
 *     bufsize: Size of buffer (should be at least 200).
 */
static void rtl_describe_alias(const RTLUnit *unit, int index,
                               char *buf, int bufsize)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(index > 0);
    ASSERT(index < unit->next_alias);
    ASSERT(buf != NULL);

    const RTLAlias * const alias = &unit->aliases[index];
    char *s = buf;
    char * const top = buf + bufsize;

    s += snprintf_assert(s, top - s, "Alias %d: %s",
                         index, rtl_type_name(alias->type));
    if (alias->base) {
        s += snprintf_assert(s, top - s, " @ %d(r%d)",
                             alias->offset, alias->base);
    } else {
        s += snprintf_assert(s, top - s, ", no bound storage");
    }
    s += snprintf_assert(s, top - s, "\n");
}

/*-----------------------------------------------------------------------*/

/**
 * rtl_describe_block:  Generate a string describing the given basic block.
 *
 * [Parameters]
 *     unit: RTLUnit containing basic block to describe.
 *     index: Index of basic block.
 *     buf: Buffer into which to store result text.
 *     bufsize: Size of buffer (should be at least 200; more may be needed
 *         for blocks with many entering edges).
 */
static void rtl_describe_block(const RTLUnit *unit, int index,
                               char *buf, int bufsize)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(index >= 0);
    ASSERT(index < unit->num_blocks);
    ASSERT(buf != NULL);

    const RTLBlock * const block = &unit->blocks[index];
    char *s = buf;
    char * const top = buf + bufsize;

    s += snprintf_assert(s, top - s, "Block %d: ", index);

    if (block->entries[0] < 0 && block->entry_overflow < 0) {
        s += snprintf_assert(s, top - s, "<none>");
    } else {
        const char *comma = "";
        for (int entry_index = index; entry_index >= 0;
             entry_index = unit->blocks[entry_index].entry_overflow)
        {
            const RTLBlock * const entry_block = &unit->blocks[entry_index];
            for (int j = 0; (j < lenof(entry_block->entries)
                             && entry_block->entries[j] >= 0); j++) {
                s += snprintf_assert(s, top - s, "%s%d", comma,
                                     entry_block->entries[j]);
                comma = ",";
            }
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
            s += snprintf_assert(s, top - s, "%s%d",
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
    unit->have_block = false;
    unit->cur_block = 0;
    unit->last_block = -1;

    unit->regs = NULL;
    unit->regs_size = REGS_EXPAND_SIZE;
    unit->next_reg = 1;
    unit->first_live_reg = 0;
    unit->last_live_reg = 0;
    unit->membase_reg = 0;

    unit->aliases = NULL;
    unit->aliases_size = REGS_EXPAND_SIZE;
    unit->next_alias = 1;

    unit->label_blockmap = NULL;
    unit->labels_size = LABELS_EXPAND_SIZE;
    unit->next_label = 1;

    unit->error = false;
    unit->finalized = false;

    unit->disassembly = NULL;

    unit->insns = rtl_malloc(unit, sizeof(*unit->insns) * unit->insns_size);
    if (!unit->insns) {
        log_error(handle, "No memory for %u instructions", unit->insns_size);
        goto fail;
    }
    memset(unit->insns, 0, sizeof(*unit->insns) * unit->insns_size);

    unit->blocks = rtl_malloc(unit, sizeof(*unit->blocks) * unit->blocks_size);
    if (!unit->blocks) {
        log_error(handle, "No memory for %u blocks", unit->blocks_size);
        goto fail;
    }
    memset(unit->blocks, 0, sizeof(*unit->blocks) * unit->blocks_size);

    unit->regs = rtl_malloc(unit, sizeof(*unit->regs) * unit->regs_size);
    if (!unit->regs) {
        log_error(handle, "No memory for %u registers", unit->regs_size);
        goto fail;
    }
    memset(&unit->regs[0], 0, sizeof(*unit->regs) * unit->regs_size);

    unit->aliases =
        rtl_malloc(unit, sizeof(*unit->aliases) * unit->aliases_size);
    if (!unit->aliases) {
        log_error(handle, "No memory for %u alias registers", unit->regs_size);
        goto fail;
    }
    memset(&unit->aliases[0], 0, sizeof(*unit->aliases) * unit->aliases_size);

    unit->label_blockmap =
        rtl_malloc(unit, sizeof(*unit->label_blockmap) * unit->labels_size);
    if (!unit->label_blockmap) {
        log_error(handle, "No memory for %u labels", unit->labels_size);
        goto fail;
    }
    memset(&unit->label_blockmap[0], -1,
           sizeof(*unit->label_blockmap) * unit->labels_size);

    return unit;

  fail:
    rtl_free(unit, unit->insns);
    rtl_free(unit, unit->blocks);
    rtl_free(unit, unit->regs);
    rtl_free(unit, unit->aliases);
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
    unit->have_block = false;
    unit->cur_block = 0;

    unit->next_reg = 1;
    unit->first_live_reg = 0;
    unit->last_live_reg = 0;

    unit->next_alias = 1;

    unit->next_label = 1;

    unit->error = false;
    unit->finalized = false;
}

/*-----------------------------------------------------------------------*/

bool rtl_get_error_state(RTLUnit *unit)
{
    return unit->error;
}

/*-----------------------------------------------------------------------*/

void rtl_clear_error_state(RTLUnit *unit)
{
    unit->error = false;
}

/*-----------------------------------------------------------------------*/

bool rtl_add_insn(RTLUnit *unit, RTLOpcode opcode,
                  int dest, int src1, int src2, uint64_t other)
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
        unit->error = true;
        return false;
    }

    unit->num_insns++;
    return true;
}

/*-----------------------------------------------------------------------*/

int rtl_add_chain_insn(RTLUnit *unit, int src1, int src2)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);

    const int insn_index = unit->num_insns;
    if (!rtl_add_insn(unit, RTLOP_CHAIN, 0, src1, src2, 0)) {
        return -1;
    }
    return insn_index;
}

/*-----------------------------------------------------------------------*/

int rtl_alloc_register(RTLUnit *unit, RTLDataType type)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);

    if (UNLIKELY(unit->next_reg >= unit->regs_size)) {
        if (unit->regs_size >= REGS_LIMIT) {
            log_error(unit->handle, "Too many registers in unit (limit %u)",
                      REGS_LIMIT);
            unit->error = true;
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
            unit->error = true;
            return 0;
        }
        memset(&new_regs[unit->regs_size], 0,
               sizeof(*new_regs) * (new_regs_size - unit->regs_size));
        unit->regs = new_regs;
        unit->regs_size = new_regs_size;
    }

    const int reg_index = unit->next_reg++;
    RTLRegister *reg = &unit->regs[reg_index];
    reg->type = type;
    return reg_index;
}

/*-----------------------------------------------------------------------*/

void rtl_make_unfoldable(RTLUnit *unit, int reg)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);

    if (UNLIKELY(reg == 0 || reg >= unit->next_reg)) {
        log_error(unit->handle, "rtl_make_unfoldable: Invalid register %d",
                  reg);
        return;
    }
    if (unit->regs[reg].source == RTLREG_CONSTANT) {
        unit->regs[reg].source = RTLREG_CONSTANT_NOFOLD;
    } else if (unit->regs[reg].source == RTLREG_RESULT) {
        unit->regs[reg].source = RTLREG_RESULT_NOFOLD;
    } else {
        log_error(unit->handle, "rtl_make_unfoldable: Register %d has"
                  " invalid source %s (must be constant or result)", reg,
                  rtl_source_name(unit->regs[reg].source));
    }
}

/*-----------------------------------------------------------------------*/

void rtl_make_unspillable(RTLUnit *unit, int reg)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);

    if (UNLIKELY(reg == 0 || reg >= unit->next_reg)) {
        log_error(unit->handle, "rtl_make_unspillable: Invalid register %d",
                  reg);
        return;
    }
    unit->regs[reg].unspillable = true;
}

/*-----------------------------------------------------------------------*/

void rtl_make_unique_pointer(RTLUnit *unit, int reg)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);

    if (UNLIKELY(reg == 0 || reg >= unit->next_reg)) {
        log_error(unit->handle, "rtl_make_unique_pointer: Invalid register %d",
                  reg);
        return;
    }
    if (UNLIKELY(unit->regs[reg].type != RTLTYPE_ADDRESS)) {
        log_error(unit->handle, "rtl_make_unique_pointer: Register %d has"
                  " invalid type %s (must be address)", reg,
                  rtl_type_name(unit->regs[reg].type));
        return;
    }

    unit->regs[reg].unique_pointer = reg;
}

/*-----------------------------------------------------------------------*/

int rtl_alloc_alias_register(RTLUnit *unit, RTLDataType type)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->aliases != NULL);

    if (UNLIKELY(unit->next_alias >= unit->aliases_size)) {
        if (unit->aliases_size >= ALIASES_LIMIT) {
            log_error(unit->handle, "Too many alias registers in unit"
                      " (limit %u)", ALIASES_LIMIT);
            unit->error = true;
            return 0;
        }
        unsigned int new_aliases_size;
        if (unit->aliases_size > ALIASES_LIMIT - ALIASES_EXPAND_SIZE) {
            new_aliases_size = ALIASES_LIMIT;
        } else {
            new_aliases_size = unit->next_alias + ALIASES_EXPAND_SIZE;
        }
        RTLAlias * const new_aliases = rtl_realloc(
            unit, unit->aliases, sizeof(*unit->aliases) * new_aliases_size);
        if (UNLIKELY(!new_aliases)) {
            log_error(unit->handle, "No memory to expand unit to %u alias"
                      " registers", new_aliases_size);
            unit->error = true;
            return 0;
        }
        memset(&new_aliases[unit->aliases_size], 0,
               sizeof(*new_aliases) * (new_aliases_size - unit->aliases_size));
        unit->aliases = new_aliases;
        unit->aliases_size = new_aliases_size;
    }

    const int alias_index = unit->next_alias++;
    unit->aliases[alias_index].type = type;
    return alias_index;
}

/*-----------------------------------------------------------------------*/

void rtl_set_alias_storage(RTLUnit *unit, int alias, int base, int16_t offset)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->aliases != NULL);

    if (UNLIKELY(alias == 0 || alias >= unit->next_alias)) {
        log_error(unit->handle, "rtl_set_alias_storage: Invalid alias %d",
                  alias);
        return;
    }
    if (UNLIKELY(base == 0 || base >= unit->next_reg)) {
        log_error(unit->handle, "rtl_set_alias_storage: Invalid register %d",
                  base);
        return;
    }
    if (UNLIKELY(unit->regs[base].type != RTLTYPE_ADDRESS)) {
        log_error(unit->handle, "rtl_set_alias_storage: Register %d has"
                  " invalid type (must be ADDRESS)", base);
        return;
    }

    unit->aliases[alias].base = base;
    unit->aliases[alias].offset = offset;
    unit->regs[base].unspillable = true;
}

/*-----------------------------------------------------------------------*/

int rtl_alloc_label(RTLUnit *unit)
{
    ASSERT(unit != NULL);
    ASSERT(!unit->finalized);
    ASSERT(unit->label_blockmap != NULL);

    if (UNLIKELY(unit->next_label >= unit->labels_size)) {
        if (unit->labels_size >= LABELS_LIMIT) {
            log_error(unit->handle, "Too many labels in unit (limit %u)",
                      LABELS_LIMIT);
            unit->error = true;
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
            unit->error = true;
            return 0;
        }
        memset(&new_label_blockmap[unit->labels_size], -1,
               sizeof(*new_label_blockmap) * (new_labels_size - unit->labels_size));
        unit->label_blockmap = new_label_blockmap;
        unit->labels_size = new_labels_size;
    }

    const int label = unit->next_label++;
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
        unit->have_block = false;
    }

    /* Add execution graph edges for GOTO instructions. */
    if (UNLIKELY(!add_block_edges(unit))) {
        return false;
    }

    unit->finalized = true;
    return true;
}

/*-----------------------------------------------------------------------*/

void rtl_set_membase_pointer(RTLUnit *unit, int reg)
{
    ASSERT(unit != NULL);

    if (UNLIKELY(reg == 0 || reg >= unit->next_reg)) {
        log_error(unit->handle, "rtl_set_membase_pointer: Invalid register %d",
                  reg);
        return;
    }
    if (UNLIKELY(unit->regs[reg].type != RTLTYPE_ADDRESS)) {
        log_error(unit->handle, "rtl_set_membase_pointer: Register %d has"
                  " invalid type %s (must be address)", reg,
                  rtl_type_name(unit->regs[reg].type));
        return;
    }
    if (UNLIKELY(unit->regs[reg].source == RTLREG_UNDEFINED)) {
        log_error(unit->handle, "rtl_set_membase_pointer: Register %d is"
                  " not yet defined", reg);
        return;
    }

    unit->membase_reg = reg;

    /* Make it unfoldable (if necessary) so optimization doesn't eliminate
     * it and prevent read-only load detection. */
    if (unit->regs[reg].source == RTLREG_CONSTANT) {
        unit->regs[reg].source = RTLREG_CONSTANT_NOFOLD;
    } else if (unit->regs[reg].source == RTLREG_RESULT) {
        unit->regs[reg].source = RTLREG_RESULT_NOFOLD;
    }
}

/*-----------------------------------------------------------------------*/

bool rtl_optimize_unit(RTLUnit *unit, unsigned int flags)
{
    ASSERT(unit != NULL);
    ASSERT(unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);

    /* Allocate and clear a buffer for block "seen" flags, used by some
     * optimizers.  We triple the size to make room for entry overflow
     * blocks which might be added by branch threading (this is overkill,
     * but it's guaranteed to be enough space because at most one overflow
     * block can be added per block exit, and there are at most two exits
     * per block). */
    unit->block_seen_size = 3 * unit->num_blocks;
    unit->block_seen =
        rtl_malloc(unit, unit->block_seen_size * sizeof(*unit->block_seen));
    if (UNLIKELY(!unit->block_seen)) {
        log_error(unit->handle, "Failed to allocate blocks_seen (%d bytes)",
                  (int)(unit->block_seen_size * sizeof(*unit->block_seen)));
        return false;
    }

    /* Perform optimizations in the proper order. */
    if (flags & (BINREC_OPT_FOLD_CONSTANTS | BINREC_OPT_FOLD_VECTORS)) {
        rtl_opt_fold_registers(unit,
                               (flags & BINREC_OPT_FOLD_CONSTANTS) != 0,
                               (flags & BINREC_OPT_FOLD_FP_CONSTANTS) != 0,
                               (flags & BINREC_OPT_FOLD_VECTORS) != 0);
    }
    if (flags & BINREC_OPT_DECONDITION) {
        rtl_opt_decondition(unit);
    }
    if (flags & BINREC_OPT_DEEP_DATA_FLOW) {
        rtl_opt_alias_data_flow(unit);
    }
    if (flags & BINREC_OPT_DSE) {
        rtl_opt_drop_dead_stores(unit, (flags & BINREC_OPT_DSE_FP) != 0);
    }
    if (flags & BINREC_OPT_BASIC) {
        rtl_opt_thread_branches(unit);
        rtl_opt_drop_dead_blocks(unit);
        rtl_opt_drop_dead_branches(unit, (flags & BINREC_OPT_DSE_FP) != 0,
                                   (flags & BINREC_OPT_DSE) != 0);
    }

    /* Free the "seen" flag buffer before returning. */
    rtl_free(unit, unit->block_seen);
    unit->block_seen = NULL;

    return true;
}

/*-----------------------------------------------------------------------*/

bool rtl_verify_unit(RTLUnit *unit, uint32_t address)
{
    ASSERT(unit != NULL);
    ASSERT(unit->finalized);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);

    bool error = false;

    /* Check that all registers used as instruction inputs are properly set. */
    for (unsigned int i = 0; i < unit->num_insns; i++) {
        const RTLInsn *insn = &unit->insns[i];
        if (insn->src1 && !verify_reg(unit, insn->src1, address, i)) {
            error = true;
        }
        if (insn->src2 && !verify_reg(unit, insn->src2, address, i)) {
            error = true;
        }
        if (insn->opcode == RTLOP_SELECT
         || insn->opcode == RTLOP_FMADD
         || insn->opcode == RTLOP_FMSUB
         || insn->opcode == RTLOP_FNMADD
         || insn->opcode == RTLOP_FNMSUB
         || insn->opcode == RTLOP_CMPXCHG
         || insn->opcode == RTLOP_CALL
         || insn->opcode == RTLOP_CALL_TRANSPARENT) {
            if (insn->src3 && !verify_reg(unit, insn->src3, address, i)) {
                error = true;
            }
        }
    }

    return !error;
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

    if (unit->next_alias > 1) {
        for (unsigned int index = 1; index < unit->next_alias; index++) {
            char text[200];
            rtl_describe_alias(unit, index, text, sizeof(text));
            const int text_len = strlen(text);
            /* +2 instead of +1 for the following newline, as above. */
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
            buflen += snprintf(&buf[buflen], bufsize - buflen, "%s", text);
            ASSERT(buflen + 1 <= bufsize);
        }

        buf[buflen++] = '\n';
        ASSERT(buflen < bufsize);
        buf[buflen] = '\0';
    }

    for (int index = 0; index != -1; index = unit->blocks[index].next_block) {
        char text[4000];
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
        rtl_free(unit, unit->aliases);
        rtl_free(unit, unit->label_blockmap);
        rtl_free(unit, unit->disassembly);
        rtl_free(unit, unit);
    }
}

/*************************************************************************/
/*************************************************************************/
