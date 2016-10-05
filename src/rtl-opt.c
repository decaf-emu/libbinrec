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
#include "src/endian.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * mulh64:  Multiply two signed or unsigned 64-bit integers and return the
 * high 64 bits of the 128-bit product.
 */
static inline uint64_t mulh64(uint64_t a, uint64_t b, bool is_signed)
{
    /* This would be easy if we had an int128_t, but for portability's sake
     * we assume we don't, so we compute the high part of the result by
     * splitting into halves.  For example, one can compute the product of
     * two 2-digit base-10 integers "ab"*"cd" (where a, b, c, and d are
     * digits) by rewriting the 2-digit numbers in the form (10a+b) and
     * (10c+d); then (10a+b)(10c+d) = 100ac + 10ad + 10bc + bd, and the
     * high two digits of the result are ac + ((ad + bc + bd/10) / 10)
     * using truncating division.  Note that the product of two N-digit
     * numbers (in any base) will never have more than N*2 digits. */

    const bool a_sign = is_signed && a>>63;
    const bool b_sign = is_signed && b>>63;
    const uint64_t a_abs = a_sign ? -a : a;
    const uint64_t b_abs = b_sign ? -b : b;
    const uint64_t high = (a_abs>>32) * (b_abs>>32);
    const uint64_t mid1 = (a_abs>>32) * (uint64_t)(uint32_t)b_abs;
    const uint64_t mid2 = (uint64_t)(uint32_t)a_abs * (b_abs>>32);
    const uint64_t low = (uint64_t)(uint32_t)a_abs * (uint64_t)(uint32_t)b_abs;

    uint64_t result_high = high + ((mid1 + mid2 + (low>>32)) >> 32);
    if (a_sign != b_sign) {
        result_high = -result_high;
        const uint64_t result_low = a * b;
        if (result_low != 0) {
            result_high -= 1;
        }
    }

    return result_high;
}

/*-----------------------------------------------------------------------*/

/**
 * reg_value_i64:  Return the value of the given constant register as a
 * 64-bit integer.  Floating-point types are bitcast to integer.
 */
static inline uint64_t reg_value_i64(const RTLRegister * const reg)
{
    if (reg->type == RTLTYPE_FLOAT) {
        return float_to_bits(reg->value.f32);
    } else {
        return reg->value.i64;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * rollback_reg_death:  Roll back the given register's death time to the
 * previous instruction (before reg->death) at which it is referenced.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg: Register to attempt to eliminate.
 *     reg_index: Index of register in unit->regs[].
 */
static void rollback_reg_death(
    RTLUnit * const unit, RTLRegister * const reg, const int reg_index)
{
    ASSERT(unit);
    ASSERT(reg);
    ASSERT(reg->live);

    for (int insn_index = reg->death-1; insn_index > reg->birth; insn_index--) {
        const RTLInsn * const insn = &unit->insns[insn_index];
        switch ((RTLOpcode)insn->opcode) {

          case RTLOP_GET_ALIAS:
          case RTLOP_LOAD_IMM:
          case RTLOP_LOAD_ARG:
          case RTLOP_LABEL:
          case RTLOP_GOTO:
          case RTLOP_ILLEGAL:
            break;

          case RTLOP_SET_ALIAS:
          case RTLOP_MOVE:
          case RTLOP_SCAST:
          case RTLOP_ZCAST:
          case RTLOP_SEXT8:
          case RTLOP_SEXT16:
          case RTLOP_NEG:
          case RTLOP_NOT:
          case RTLOP_CLZ:
          case RTLOP_BSWAP:
          case RTLOP_BFEXT:
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
          case RTLOP_LOAD:
          case RTLOP_LOAD_U8:
          case RTLOP_LOAD_S8:
          case RTLOP_LOAD_U16:
          case RTLOP_LOAD_S16:
          case RTLOP_LOAD_BR:
          case RTLOP_LOAD_U16_BR:
          case RTLOP_LOAD_S16_BR:
          case RTLOP_ATOMIC_INC:
          case RTLOP_GOTO_IF_Z:
          case RTLOP_GOTO_IF_NZ:
          case RTLOP_RETURN:
            if (insn->src1 == reg_index) {
              still_used:
#ifdef RTL_DEBUG_OPTIMIZE
                log_info(unit->handle, "[RTL] rollback_reg_death: r%d still"
                         " used at insn %d\n", reg_index, insn_index);
#endif
                reg->death = insn_index;
                return;
            }
            break;

          case RTLOP_NOP:
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
          case RTLOP_BFINS:
          case RTLOP_STORE:
          case RTLOP_STORE_I8:
          case RTLOP_STORE_I16:
          case RTLOP_STORE_BR:
          case RTLOP_STORE_I16_BR:
            if (insn->src1 == reg_index || insn->src2 == reg_index) {
                goto still_used;
            }
            break;

          case RTLOP_SELECT:
          case RTLOP_CMPXCHG:
          case RTLOP_CALL_ADDR:
            if (insn->src1 == reg_index || insn->src2 == reg_index
             || insn->src3 == reg_index) {
                goto still_used;
            }
            break;

        }  // switch (opcode)
    }  // for (insn_index)

    /* If we got this far, nothing else uses the register. */
#ifdef RTL_DEBUG_OPTIMIZE
    log_info(unit->handle, "[RTL] rollback_reg_death: r%d no longer used,"
             " setting death = birth\n", reg_index);
#endif
    reg->death = reg->birth;
}

/*-----------------------------------------------------------------------*/

/**
 * fold_constant:  Perform the operation specified by reg->result and
 * return the result value in 64 bits (bitcast from floating-point if
 * necessary).  Helper for fold_one_register.
 */
static inline uint64_t fold_constant(RTLUnit * const unit,
                                     RTLRegister * const reg)
{
    RTLRegister * const src1 = &unit->regs[reg->result.src1];
    RTLRegister * const src2 = &unit->regs[reg->result.src2];

    switch ((RTLOpcode)reg->result.opcode) {

      case RTLOP_MOVE:
        return reg_value_i64(src1);
        break;

      case RTLOP_SELECT:
        return (unit->regs[reg->result.src3].value.i64
                ? src1->value.i64 : src2->value.i64);
        break;

      case RTLOP_SCAST:
        if (src1->type == RTLTYPE_INT32) {
            return (int32_t)src1->value.i64;
        } else {
            return src1->value.i64;
        }
        break;

      case RTLOP_ZCAST:
        if (src1->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64;
        } else {
            return src1->value.i64;
        }
        break;

      case RTLOP_SEXT8:
        return (int8_t)src1->value.i64;
        break;

      case RTLOP_SEXT16:
        return (int8_t)src1->value.i64;
        break;

      case RTLOP_ADD:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 + (uint32_t)src2->value.i64;
        } else {
            return src1->value.i64 + src2->value.i64;
        }
        break;

      case RTLOP_SUB:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 - (uint32_t)src2->value.i64;
        } else {
            return src1->value.i64 - src2->value.i64;
        }
        break;

      case RTLOP_NEG:
        if (reg->type == RTLTYPE_INT32) {
            return -(uint32_t)src1->value.i64;
        } else {
            return -src1->value.i64;
        }
        break;

      case RTLOP_MUL:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 * (uint32_t)src2->value.i64;
        } else {
            return src1->value.i64 * src2->value.i64;
        }
        break;

      case RTLOP_MULHU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint64_t)(uint32_t)src1->value.i64
                    * (uint64_t)(uint32_t)src2->value.i64) >> 32;
        } else {
            return mulh64(src1->value.i64, src2->value.i64, false);
        }
        break;

      case RTLOP_MULHS:
        if (reg->type == RTLTYPE_INT32) {
            return ((int64_t)(int32_t)src1->value.i64
                      * (int64_t)(int32_t)src2->value.i64) >> 32;
        } else {
            return mulh64(src1->value.i64, src2->value.i64, true);
        }
        break;

      case RTLOP_DIVU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src2->value.i64
                    ? (uint32_t)src1->value.i64 / (uint32_t)src2->value.i64 : 0);
        } else {
            return (src2->value.i64 ? src1->value.i64 / src2->value.i64 : 0);
        }
        break;

      case RTLOP_DIVS:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src2->value.i64
                    && !((uint32_t)src1->value.i64 == 1u<<31
                         && (int32_t)src2->value.i64 == -1)
                    ? (int32_t)src1->value.i64 / (int32_t)src2->value.i64 : 0);
        } else {
            return (src2->value.i64
                    && !(src1->value.i64 == UINT64_C(1)<<63
                         && src2->value.i64 == UINT64_C(-1))
                    ? (int64_t)src1->value.i64 / (int64_t)src2->value.i64 : 0);
        }
        break;

      case RTLOP_MODU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src2->value.i64
                    ? (uint32_t)src1->value.i64 % (uint32_t)src2->value.i64 : 0);
        } else {
            return (src2->value.i64 ? src1->value.i64 % src2->value.i64 : 0);
        }
        break;

      case RTLOP_MODS:
        if (reg->type == RTLTYPE_INT32) {
            return (abs((int32_t)src2->value.i64) > 1
                    ? (int32_t)src1->value.i64 % (int32_t)src2->value.i64 : 0);
        } else {
            return (llabs((int64_t)src2->value.i64) > 1
                    ? (int64_t)src1->value.i64 % (int64_t)src2->value.i64 : 0);
        }
        break;

      case RTLOP_AND:
        /* AND, OR, and XOR always return the same number of bits as the
         * input, so we don't need to explicitly cast down to uint32_t. */
        return src1->value.i64 & src2->value.i64;
        break;

      case RTLOP_OR:
        return src1->value.i64 | src2->value.i64;
        break;

      case RTLOP_XOR:
        return src1->value.i64 ^ src2->value.i64;
        break;

      case RTLOP_NOT:
        if (reg->type == RTLTYPE_INT32) {
            return ~(uint32_t)src1->value.i64;
        } else {
            return ~src1->value.i64;
        }
        break;

      case RTLOP_SLL:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 << src2->value.i64;
        } else {
            return src1->value.i64 << src2->value.i64;
        }
        break;

      case RTLOP_SRL:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 >> src2->value.i64;
        } else {
            return src1->value.i64 >> src2->value.i64;
        }
        break;

      case RTLOP_SRA:
        if (reg->type == RTLTYPE_INT32) {
            return (int32_t)src1->value.i64 >> src2->value.i64;
        } else {
            return (int64_t)src1->value.i64 >> src2->value.i64;
        }
        break;

      case RTLOP_ROL:
        if (reg->type == RTLTYPE_INT32) {
            return ror32((uint32_t)src1->value.i64, -(int)src2->value.i64);
        } else {
            return ror64(src1->value.i64, -(int)src2->value.i64);
        }
        break;

      case RTLOP_ROR:
        if (reg->type == RTLTYPE_INT32) {
            return ror32((uint32_t)src1->value.i64, (int)src2->value.i64);
        } else {
            return ror64(src1->value.i64, (int)src2->value.i64);
        }
        break;

      case RTLOP_CLZ:
        if (reg->type == RTLTYPE_INT32) {
            return clz32((uint32_t)src1->value.i64);
        } else {
            return clz64(src1->value.i64);
        }
        break;

      case RTLOP_BSWAP:
        if (reg->type == RTLTYPE_INT32) {
            return bswap32((uint32_t)src1->value.i64);
        } else {
            return bswap64(src1->value.i64);
        }
        break;

      case RTLOP_SEQ:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 == (uint32_t)src2->value.i64);
        } else {
            return (src1->value.i64 == src2->value.i64);
        }
        break;

      case RTLOP_SLTU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 < (uint32_t)src2->value.i64);
        } else {
            return (src1->value.i64 < src2->value.i64);
        }
        break;

      case RTLOP_SLTS:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 < (int32_t)src2->value.i64);
        } else {
            return ((int64_t)src1->value.i64 < (int64_t)src2->value.i64);
        }
        break;

      case RTLOP_SGTU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 > (uint32_t)src2->value.i64);
        } else {
            return (src1->value.i64 > src2->value.i64);
        }
        break;

      case RTLOP_SGTS:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 > (int32_t)src2->value.i64);
        } else {
            return ((int64_t)src1->value.i64 > (int64_t)src2->value.i64);
        }
        break;

      case RTLOP_BFEXT:
        if (reg->type == RTLTYPE_INT32) {
            return (((uint32_t)src1->value.i64 >> reg->result.start)
                    & ((1u << reg->result.count) - 1));
        } else {
            return ((src1->value.i64 >> reg->result.start)
                    & ((UINT64_C(1) << reg->result.count) - 1));
        }
        return 1;

      case RTLOP_BFINS:
        if (reg->type == RTLTYPE_INT32) {
            const uint32_t mask =
                ((1u << reg->result.count) - 1) << reg->result.start;
            return (src1->value.i64 & ~mask)
                 | ((src2->value.i64 << reg->result.start) & mask);
        } else {
            const uint64_t mask =
                ((UINT64_C(1) << reg->result.count) - 1) << reg->result.start;
            return (src1->value.i64 & ~mask)
                 | ((src2->value.i64 << reg->result.start) & mask);
        }
        return 1;

      case RTLOP_ADDI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 + reg->result.src_imm;
        } else {
            return src1->value.i64 + reg->result.src_imm;
        }
        break;

      case RTLOP_MULI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 * reg->result.src_imm;
        } else {
            return src1->value.i64 * reg->result.src_imm;
        }
        break;

      case RTLOP_ANDI:
        /* Unlike the register-register variant, we have to handle int32
         * separately here because of sign-extension of the immediate value. */
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 & (uint32_t)reg->result.src_imm;
        } else {
            return src1->value.i64 & (uint64_t)reg->result.src_imm;
        }
        break;

      case RTLOP_ORI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 | (uint32_t)reg->result.src_imm;
        } else {
            return src1->value.i64 | (uint64_t)reg->result.src_imm;
        }
        break;

      case RTLOP_XORI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 ^ (uint32_t)reg->result.src_imm;
        } else {
            return src1->value.i64 ^ (uint64_t)reg->result.src_imm;
        }
        break;

      case RTLOP_SLLI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 << reg->result.src_imm;
        } else {
            return src1->value.i64 << reg->result.src_imm;
        }
        break;

      case RTLOP_SRLI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 >> reg->result.src_imm;
        } else {
            return src1->value.i64 >> reg->result.src_imm;
        }
        break;

      case RTLOP_SRAI:
        if (reg->type == RTLTYPE_INT32) {
            return (int32_t)src1->value.i64 >> reg->result.src_imm;
        } else {
            return (int64_t)src1->value.i64 >> reg->result.src_imm;
        }
        break;

      case RTLOP_RORI:
        if (reg->type == RTLTYPE_INT32) {
            return ror32((uint32_t)src1->value.i64, reg->result.src_imm);
        } else {
            return ror64(src1->value.i64, reg->result.src_imm);
        }
        break;

      case RTLOP_SEQI:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 == (uint32_t)reg->result.src_imm);
        } else {
            return (src1->value.i64 == (uint64_t)reg->result.src_imm);
        }
        break;

      case RTLOP_SLTUI:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 < (uint32_t)reg->result.src_imm);
        } else {
            return (src1->value.i64 < (uint64_t)reg->result.src_imm);
        }
        break;

      case RTLOP_SLTSI:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 < (int32_t)reg->result.src_imm);
        } else {
            return ((int64_t)src1->value.i64 < (int64_t)reg->result.src_imm);
        }
        break;

      case RTLOP_SGTUI:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 > (uint32_t)reg->result.src_imm);
        } else {
            return (src1->value.i64 > (uint64_t)reg->result.src_imm);
        }
        break;

      case RTLOP_SGTSI:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 > (int32_t)reg->result.src_imm);
        } else {
            return ((int64_t)src1->value.i64 > (int64_t)reg->result.src_imm);
        }
        break;

      /* The remainder will never appear with RTLREG_RESULT, but list them
       * individually rather than using a default case so the compiler will
       * warn us if we add a new opcode but don't include it here. */
      //FIXME: notimp: read constants from read-only memory
      case RTLOP_NOP:
      case RTLOP_SET_ALIAS:
      case RTLOP_GET_ALIAS:
      case RTLOP_ATOMIC_INC:
      case RTLOP_CMPXCHG:
      case RTLOP_LOAD_IMM:
      case RTLOP_LOAD_ARG:
      case RTLOP_LOAD:
      case RTLOP_LOAD_U8:
      case RTLOP_LOAD_S8:
      case RTLOP_LOAD_U16:
      case RTLOP_LOAD_S16:
      case RTLOP_LOAD_BR:
      case RTLOP_LOAD_U16_BR:
      case RTLOP_LOAD_S16_BR:
      case RTLOP_STORE:
      case RTLOP_STORE_I8:
      case RTLOP_STORE_I16:
      case RTLOP_STORE_BR:
      case RTLOP_STORE_I16_BR:
      case RTLOP_LABEL:
      case RTLOP_GOTO:
      case RTLOP_GOTO_IF_Z:
      case RTLOP_GOTO_IF_NZ:
      case RTLOP_CALL_ADDR:
      case RTLOP_RETURN:
      case RTLOP_ILLEGAL:
        log_error(unit->handle, "Invalid opcode %u on RESULT register %d",
                  reg->result.opcode, (int)(reg - unit->regs));
        return 0;

    }  // switch ((RTLOpcode)reg->result.opcode)

    UNREACHABLE;
}

/*-----------------------------------------------------------------------*/

/**
 * fold_one_register:  Attempt to perform constant folding on a register.
 * The register must be of type RTLREG_RESULT.
 *
 * This routine calls itself recursively, but it is declared inline anyway
 * to optimize the outer loop in rtl_opt_fold_constants().
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 *     reg: Register on which to perform constant folding.
 * [Return value]
 *     True if constant folding was performed, false if not.
 */
static inline int fold_one_register(RTLUnit * const unit,
                                    RTLRegister * const reg)
{
    ASSERT(unit);
    ASSERT(reg);
    ASSERT(reg->live);
    ASSERT(reg->source == RTLREG_RESULT);

    /* Flag the register as not foldable for the moment, to avoid infinite
     * recursion in case invalid code creates register dependency loops. */
    reg->source = RTLREG_RESULT_NOFOLD;

    /* See if the operands are constant, folding them if necessary. */
    RTLRegister * const src1 = &unit->regs[reg->result.src1];
    RTLRegister * const src2 = &unit->regs[reg->result.src2];
    if (src1->source != RTLREG_CONSTANT
     && !(src1->source == RTLREG_RESULT
          && fold_one_register(unit, src1))) {
        return false;  // Operand 1 wasn't constant.
    }
    if (!reg->result.is_imm
     && reg->result.src2 != 0  // In case it's a 1-operand instruction.
     && src2->source != RTLREG_CONSTANT
     && !(src2->source == RTLREG_RESULT
          && fold_one_register(unit, src2))) {
        return false;  // Operand 2 wasn't constant.
    }
    if (reg->result.opcode == RTLOP_SELECT) {
        RTLRegister * const src3 = &unit->regs[reg->result.src3];
        if (src3->source != RTLREG_CONSTANT
         && !(src3->source == RTLREG_RESULT
              && fold_one_register(unit, src3))) {
            return false;  // Operand 3 wasn't constant.
        }
    }

    /* All operands are constants, so perform the operation now and convert
     * the register to a constant. */
    const uint64_t result = fold_constant(unit, reg);
#ifdef RTL_DEBUG_OPTIMIZE
    log_info(unit->handle, "[RTL] Folded r%d to constant value 0x%"PRIX64" at"
             " insn %d\n", (int)(reg - unit->regs), result, reg->birth);
#endif

    /* Rewrite the instruction that sets this register into a LOAD_IMM
     * with the result we calculated. */
    unit->insns[reg->birth].opcode = RTLOP_LOAD_IMM;
    unit->insns[reg->birth].src1 = 0;
    unit->insns[reg->birth].src2 = 0;
    unit->insns[reg->birth].src_imm = result;

    /* Update death times of the source registers as appropriate. */
    if (src1->death == reg->birth) {
        rollback_reg_death(unit, src1, reg->result.src1);
    }
    if (!reg->result.is_imm && reg->result.src2 != 0
     && src2->death == reg->birth) {
        rollback_reg_death(unit, src2, reg->result.src2);
    }
    if (reg->result.opcode == RTLOP_SELECT) {
        RTLRegister * const src3 = &unit->regs[reg->result.src3];
        if (src3->death == reg->birth) {
            rollback_reg_death(unit, src3, reg->result.src3);
        }
    }

    /* Constant folding was successful. */
    reg->source = RTLREG_CONSTANT;
    reg->value.i64 = result;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * visit_block:  Mark the given block as seen and visit all successor
 * blocks.  Helper function for rtl_opt_drop_dead_blocks().
 *
 * [Parameters]
 *     unit: RTL unit.
 *     block_index: Index of block in unit->blocks[].
 */
static void visit_block(RTLUnit * const unit, const int block_index)
{
    unit->block_seen[block_index] = 1;
    RTLBlock * const block = &unit->blocks[block_index];
    for (int i = 0; i < lenof(block->exits) && block->exits[i] >= 0; i++) {
        if (!unit->block_seen[block->exits[i]]) {
            visit_block(unit, block->exits[i]);
        }
    }
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

void rtl_opt_decondition(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);

    /* A conditional branch always ends a basic block and has two targets,
     * so we only need to check the last instruction of each block with two
     * exit edges. */

    for (int block_index = 0; block_index < unit->num_blocks; block_index++) {
        RTLBlock * const block = &unit->blocks[block_index];
        if (block->exits[1] != -1) {
            RTLInsn * const insn = &unit->insns[block->last_insn];
            const RTLOpcode opcode = insn->opcode;
            if (opcode == RTLOP_GOTO_IF_Z || opcode == RTLOP_GOTO_IF_NZ) {
                const int reg_index = insn->src1;
                RTLRegister * const condition_reg = &unit->regs[reg_index];
                if (condition_reg->source == RTLREG_CONSTANT) {
                    const uint64_t condition = condition_reg->value.i64;
                    const int fallthrough_index =
                        (block->exits[0] == block_index+1) ? 0 : 1;
                    if ((opcode == RTLOP_GOTO_IF_Z && condition == 0)
                     || (opcode == RTLOP_GOTO_IF_NZ && condition != 0)) {
                        /* Branch always taken: convert to GOTO */
#ifdef RTL_DEBUG_OPTIMIZE
                        fprintf(stderr, "[RTL] %u: Branch always taken,"
                                " convert to GOTO and drop edge %u->%u\n",
                                block->last_insn, block_index,
                                block->exits[fallthrough_index]);
#endif
                        insn->opcode = RTLOP_GOTO;
                        rtl_block_remove_edge(unit, block_index,
                                              fallthrough_index);
                    } else {
                        /* Branch never taken: convert to NOP */
#ifdef RTL_DEBUG_OPTIMIZE
                        fprintf(stderr, "[RTL] %u: Branch never taken,"
                                " convert to NOP and drop edge %u->%u\n",
                                block->last_insn, block_index,
                                block->exits[fallthrough_index ^ 1]);
#endif
                        insn->opcode = RTLOP_NOP;
                        insn->src_imm = 0;
                        rtl_block_remove_edge(unit, block_index,
                                              fallthrough_index ^ 1);
                    }
                    if (condition_reg->death == block->last_insn) {
                        rollback_reg_death(unit, condition_reg, reg_index);
                    }
                }
            }  // if (opcode == RTLOP_GOTO_IF_Z || opcode == RTLOP_GOTO_IF_NZ)
        }  // if (block->exits[1] != -1)
    }  // for (block_index = 0; block_index < unit->num_blocks; block_index++)
}

/*-----------------------------------------------------------------------*/

bool rtl_opt_drop_dead_blocks(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);

    /* Allocate and clear a buffer for "seen" flags. */
    unit->block_seen =
        rtl_malloc(unit, unit->num_blocks * sizeof(*unit->block_seen));
    if (UNLIKELY(!unit->block_seen)) {
        log_error(unit->handle, "Failed to allocate blocks_seen (%d bytes)",
                  (int)(unit->num_blocks * sizeof(*unit->block_seen)));
        return false;
    }
    memset(unit->block_seen, 0, unit->num_blocks * sizeof(*unit->block_seen));

    /* Recursively visit all blocks reachable from the start block. */
    visit_block(unit, 0);

    /* Remove all blocks which are not reachable. */
    for (int i = 0; i >= 0; i = unit->blocks[i].next_block) {
        RTLBlock * const block = &unit->blocks[i];
        if (!unit->block_seen[i]) {
#ifdef RTL_DEBUG_OPTIMIZE
            log_info(unit->handle, "[RTL] Dropping dead block %d (%d-%d)",
                     i, block->first_insn, block->last_insn);
#endif
            ASSERT(block->prev_block >= 0);
            unit->blocks[block->prev_block].next_block = block->next_block;
            if (block->next_block >= 0) {
                unit->blocks[block->next_block].prev_block = block->prev_block;
            }
        }
    }

    /* Free the "seen" flag buffer before returning (since the core doesn't
     * touch this field). */
    rtl_free(unit, unit->block_seen);
    unit->block_seen = NULL;  // Just for safety.

    return true;
}

/*-----------------------------------------------------------------------*/

void rtl_opt_drop_dead_branches(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);
    ASSERT(unit->label_blockmap);

    for (int block_index = 0; block_index < unit->num_blocks; block_index++) {
        RTLBlock * const block = &unit->blocks[block_index];
        if (block->last_insn >= block->first_insn) {
            RTLInsn * const insn = &unit->insns[block->last_insn];
            const RTLOpcode opcode = insn->opcode;
            if ((opcode == RTLOP_GOTO
              || opcode == RTLOP_GOTO_IF_Z
              || opcode == RTLOP_GOTO_IF_NZ)
             && unit->label_blockmap[insn->label] == block->next_block) {
#ifdef RTL_DEBUG_OPTIMIZE
                fprintf(stderr, "[RTL] %u: Dropping branch to next insn\n",
                        block->last_insn);
#endif
                if (opcode == RTLOP_GOTO_IF_Z || opcode == RTLOP_GOTO_IF_NZ) {
                    RTLRegister *src1_reg = &unit->regs[insn->src1];
                    if (src1_reg->death == block->last_insn) {
                        rollback_reg_death(unit, src1_reg, insn->src1);
                    }
                }
                insn->opcode = RTLOP_NOP;
                insn->src_imm = 0;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

void rtl_opt_fold_constants(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->regs);

    for (int reg_index = 1; reg_index < unit->next_reg; reg_index++) {
        RTLRegister * const reg = &unit->regs[reg_index];
        if (reg->live && reg->source == RTLREG_RESULT) {
            fold_one_register(unit, reg);
        }
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
