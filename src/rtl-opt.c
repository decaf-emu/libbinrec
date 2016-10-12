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
/*************************** Local data types ****************************/
/*************************************************************************/

/* Data structure used in alias data flow analysis.  One record per alias
 * is allocated for each block. */
typedef struct AliasRef {
    /* True if this alias is set in the block. */
    bool has_set;
    /* True if this alias is referenced before being set in the block. */
    bool has_get;
    /* True if the latest value set by a SET_ALIAS instruction in this
     * block is referenced by a subsequent GET_ALIAS in the same block.
     * Also true if the alias has bound storage and the latest SET_ALIAS
     * value is live at a call-type instruction (CALL or CALL_TRANSPARENT)
     * later in the same block. */
    bool set_used;
    /* Index of the last SET_ALIAS instruction for this alias in the block.
     * Only valid if has_set is true. */
    int32_t set_insn;
} AliasRef;

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
    if (reg->type == RTLTYPE_FLOAT32) {
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
 *     reg_index: Index of register to modify.
 * [Return value]
 *     True if the register is no longer referenced after being set,
 *     false otherwise.
 */
static bool rollback_reg_death(RTLUnit * const unit, const int reg_index)
{
    ASSERT(unit);
    ASSERT(reg_index);
    ASSERT(unit->regs[reg_index].live);

    RTLRegister * const reg = &unit->regs[reg_index];

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
                log_info(unit->handle, "r%d still used at %d",
                         reg_index, insn_index);
#endif
                reg->death = insn_index;
                return false;
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
          case RTLOP_CALL:
          case RTLOP_CALL_TRANSPARENT:
            if (insn->src1 == reg_index || insn->src2 == reg_index
             || insn->src3 == reg_index) {
                goto still_used;
            }
            break;

        }  // switch (opcode)
    }  // for (insn_index)

    /* If we got this far, nothing else uses the register. */
#ifdef RTL_DEBUG_OPTIMIZE
    log_info(unit->handle, "r%d no longer used, setting death = birth",
             reg_index);
#endif
    reg->death = reg->birth;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * fold_constant:  Perform the operation specified by reg->result and
 * return the result value in 64 bits (bitcast from floating-point if
 * necessary).  Helper for fold_one_register.
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 *     reg: Register to fold.
 * [Return value]
 *     Register value.
 */
static inline uint64_t fold_constant(RTLUnit * const unit,
                                     RTLRegister * const reg)
{
    RTLRegister * const src1 = &unit->regs[reg->result.src1];
    RTLRegister * const src2 = &unit->regs[reg->result.src2];

    switch ((RTLOpcode)reg->result.opcode) {

      case RTLOP_MOVE:
        return reg_value_i64(src1);

      case RTLOP_SCAST:
        if (src1->type == RTLTYPE_INT32) {
            return (int32_t)src1->value.i64;
        } else {
            return src1->value.i64;
        }

      case RTLOP_ZCAST:
        if (src1->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64;
        } else {
            return src1->value.i64;
        }

      case RTLOP_SEXT8:
        return (int8_t)src1->value.i64;

      case RTLOP_SEXT16:
        return (int16_t)src1->value.i64;

      case RTLOP_ADD:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 + (uint32_t)src2->value.i64;
        } else {
            return src1->value.i64 + src2->value.i64;
        }

      case RTLOP_SUB:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 - (uint32_t)src2->value.i64;
        } else {
            return src1->value.i64 - src2->value.i64;
        }

      case RTLOP_NEG:
        if (reg->type == RTLTYPE_INT32) {
            return -(uint32_t)src1->value.i64;
        } else {
            return -src1->value.i64;
        }

      case RTLOP_MUL:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 * (uint32_t)src2->value.i64;
        } else {
            return src1->value.i64 * src2->value.i64;
        }

      case RTLOP_MULHU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint64_t)(uint32_t)src1->value.i64
                    * (uint64_t)(uint32_t)src2->value.i64) >> 32;
        } else {
            return mulh64(src1->value.i64, src2->value.i64, false);
        }

      case RTLOP_MULHS:
        if (reg->type == RTLTYPE_INT32) {
            return ((int64_t)(int32_t)src1->value.i64
                      * (int64_t)(int32_t)src2->value.i64) >> 32;
        } else {
            return mulh64(src1->value.i64, src2->value.i64, true);
        }

      case RTLOP_DIVU:
        if (reg->type == RTLTYPE_INT32) {
            if (UNLIKELY(!(uint32_t)src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else {
                return (uint32_t)src1->value.i64 / (uint32_t)src2->value.i64;
            }
        } else {
            if (UNLIKELY(!src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else {
                return src1->value.i64 / src2->value.i64;
            }
        }

      case RTLOP_DIVS:
        if (reg->type == RTLTYPE_INT32) {
            if (UNLIKELY(!(uint32_t)src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else if (UNLIKELY((uint32_t)src1->value.i64 == 1u<<31
                                && (int32_t)src2->value.i64 == -1)) {
                log_warning(unit->handle, "r%d: Treating overflow in"
                            " constant signed division as -1<<31",
                            (int)(reg - unit->regs));
                return 0x80000000u;
            } else {
                return (int32_t)src1->value.i64 / (int32_t)src2->value.i64;
            }
        } else {
            if (UNLIKELY(!src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else if (UNLIKELY(src1->value.i64 == UINT64_C(1)<<63
                                && src2->value.i64 == UINT64_C(-1))) {
                log_warning(unit->handle, "r%d: Treating overflow in"
                            " constant signed division as -1<<63",
                            (int)(reg - unit->regs));
                return UINT64_C(1)<<63;
            } else {
                return (int64_t)src1->value.i64 / (int64_t)src2->value.i64;
            }
        }

      case RTLOP_MODU:
        if (reg->type == RTLTYPE_INT32) {
            if (UNLIKELY(!(uint32_t)src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else {
                return (uint32_t)src1->value.i64 % (uint32_t)src2->value.i64;
            }
        } else {
            if (UNLIKELY(!src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else {
                return src1->value.i64 % src2->value.i64;
            }
        }

      case RTLOP_MODS:
        if (reg->type == RTLTYPE_INT32) {
            if (UNLIKELY(!(uint32_t)src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else if (UNLIKELY((uint32_t)src1->value.i64 == 1u<<31
                                && (int32_t)src2->value.i64 == -1)) {
                log_warning(unit->handle, "r%d: Treating overflow in constant"
                            " signed division as 0", (int)(reg - unit->regs));
                return 0;
            } else {
                return (int32_t)src1->value.i64 % (int32_t)src2->value.i64;
            }
        } else {
            if (UNLIKELY(!src2->value.i64)) {
                log_warning(unit->handle, "r%d: Treating constant division"
                            " by zero as 0", (int)(reg - unit->regs));
                return 0;
            } else if (UNLIKELY(src1->value.i64 == UINT64_C(1)<<63
                                && src2->value.i64 == UINT64_C(-1))) {
                log_warning(unit->handle, "r%d: Treating overflow in constant"
                            " signed division as 0", (int)(reg - unit->regs));
                return 0;
            } else {
                return (int64_t)src1->value.i64 % (int64_t)src2->value.i64;
            }
        }

      case RTLOP_AND:
        /* AND, OR, and XOR always return the same number of bits as the
         * input, so we don't need to explicitly cast down to uint32_t. */
        return src1->value.i64 & src2->value.i64;

      case RTLOP_OR:
        return src1->value.i64 | src2->value.i64;

      case RTLOP_XOR:
        return src1->value.i64 ^ src2->value.i64;

      case RTLOP_NOT:
        if (reg->type == RTLTYPE_INT32) {
            return ~(uint32_t)src1->value.i64;
        } else {
            return ~src1->value.i64;
        }

      case RTLOP_SLL:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 << src2->value.i64;
        } else {
            return src1->value.i64 << src2->value.i64;
        }

      case RTLOP_SRL:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 >> src2->value.i64;
        } else {
            return src1->value.i64 >> src2->value.i64;
        }

      case RTLOP_SRA:
        if (reg->type == RTLTYPE_INT32) {
            return (int32_t)src1->value.i64 >> src2->value.i64;
        } else {
            return (int64_t)src1->value.i64 >> src2->value.i64;
        }

      case RTLOP_ROL:
        if (reg->type == RTLTYPE_INT32) {
            return ror32((uint32_t)src1->value.i64, -(int)src2->value.i64);
        } else {
            return ror64(src1->value.i64, -(int)src2->value.i64);
        }

      case RTLOP_ROR:
        if (reg->type == RTLTYPE_INT32) {
            return ror32((uint32_t)src1->value.i64, (int)src2->value.i64);
        } else {
            return ror64(src1->value.i64, (int)src2->value.i64);
        }

      case RTLOP_CLZ:
        if (reg->type == RTLTYPE_INT32) {
            return clz32((uint32_t)src1->value.i64);
        } else {
            return clz64(src1->value.i64);
        }

      case RTLOP_BSWAP:
        if (reg->type == RTLTYPE_INT32) {
            return bswap32((uint32_t)src1->value.i64);
        } else {
            return bswap64(src1->value.i64);
        }

      case RTLOP_SEQ:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 == (uint32_t)src2->value.i64);
        } else {
            return (src1->value.i64 == src2->value.i64);
        }

      case RTLOP_SLTU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 < (uint32_t)src2->value.i64);
        } else {
            return (src1->value.i64 < src2->value.i64);
        }

      case RTLOP_SLTS:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 < (int32_t)src2->value.i64);
        } else {
            return ((int64_t)src1->value.i64 < (int64_t)src2->value.i64);
        }

      case RTLOP_SGTU:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 > (uint32_t)src2->value.i64);
        } else {
            return (src1->value.i64 > src2->value.i64);
        }

      case RTLOP_SGTS:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 > (int32_t)src2->value.i64);
        } else {
            return ((int64_t)src1->value.i64 > (int64_t)src2->value.i64);
        }

      case RTLOP_BFEXT:
        if (reg->type == RTLTYPE_INT32) {
            return (((uint32_t)src1->value.i64 >> reg->result.start)
                    & ((1u << reg->result.count) - 1));
        } else {
            return ((src1->value.i64 >> reg->result.start)
                    & ((UINT64_C(1) << reg->result.count) - 1));
        }

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

      case RTLOP_ADDI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 + reg->result.src_imm;
        } else {
            return src1->value.i64 + reg->result.src_imm;
        }

      case RTLOP_MULI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 * reg->result.src_imm;
        } else {
            return src1->value.i64 * reg->result.src_imm;
        }

      case RTLOP_ANDI:
        /* Unlike the register-register variant, we have to handle int32
         * separately here because of sign-extension of the immediate value. */
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 & (uint32_t)reg->result.src_imm;
        } else {
            return src1->value.i64 & (uint64_t)reg->result.src_imm;
        }

      case RTLOP_ORI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 | (uint32_t)reg->result.src_imm;
        } else {
            return src1->value.i64 | (uint64_t)reg->result.src_imm;
        }

      case RTLOP_XORI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 ^ (uint32_t)reg->result.src_imm;
        } else {
            return src1->value.i64 ^ (uint64_t)reg->result.src_imm;
        }

      case RTLOP_SLLI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 << reg->result.src_imm;
        } else {
            return src1->value.i64 << reg->result.src_imm;
        }

      case RTLOP_SRLI:
        if (reg->type == RTLTYPE_INT32) {
            return (uint32_t)src1->value.i64 >> reg->result.src_imm;
        } else {
            return src1->value.i64 >> reg->result.src_imm;
        }

      case RTLOP_SRAI:
        if (reg->type == RTLTYPE_INT32) {
            return (int32_t)src1->value.i64 >> reg->result.src_imm;
        } else {
            return (int64_t)src1->value.i64 >> reg->result.src_imm;
        }

      case RTLOP_RORI:
        if (reg->type == RTLTYPE_INT32) {
            return ror32((uint32_t)src1->value.i64, reg->result.src_imm);
        } else {
            return ror64(src1->value.i64, reg->result.src_imm);
        }

      case RTLOP_SEQI:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 == (uint32_t)reg->result.src_imm);
        } else {
            return (src1->value.i64 == (uint64_t)reg->result.src_imm);
        }

      case RTLOP_SLTUI:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 < (uint32_t)reg->result.src_imm);
        } else {
            return (src1->value.i64 < (uint64_t)reg->result.src_imm);
        }

      case RTLOP_SLTSI:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 < (int32_t)reg->result.src_imm);
        } else {
            return ((int64_t)src1->value.i64 < (int64_t)reg->result.src_imm);
        }

      case RTLOP_SGTUI:
        if (reg->type == RTLTYPE_INT32) {
            return ((uint32_t)src1->value.i64 > (uint32_t)reg->result.src_imm);
        } else {
            return (src1->value.i64 > (uint64_t)reg->result.src_imm);
        }

      case RTLOP_SGTSI:
        if (reg->type == RTLTYPE_INT32) {
            return ((int32_t)src1->value.i64 > (int32_t)reg->result.src_imm);
        } else {
            return ((int64_t)src1->value.i64 > (int64_t)reg->result.src_imm);
        }

      /* The remainder will never appear with RTLREG_RESULT, but list them
       * individually rather than using a default case so the compiler will
       * warn us if we add a new opcode but don't include it here. */
      case RTLOP_NOP:
      case RTLOP_SET_ALIAS:
      case RTLOP_GET_ALIAS:
      case RTLOP_SELECT:  // Reduced to MOVE if foldable.
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
      case RTLOP_CALL:
      case RTLOP_CALL_TRANSPARENT:
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
 * convert_to_regimm:  Convert the register-register operation which sets
 * the given register to a register-immediate operation, if possible.  On
 * success, the register's result field is updated with the appropriate
 * opcode, register, and immediate values.  Helper for fold_one_register().
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 *     reg: Register to modify.
 * [Return value]
 *     True if the register's operation was modified, false if not.
 */
static inline bool convert_to_regimm(RTLUnit * const unit,
                                     RTLRegister * const reg)
{
    ASSERT(!reg->result.is_imm);
    ASSERT(reg->result.src2);
    const RTLOpcode opcode = reg->result.opcode;
    const int src1 = reg->result.src1;
    const int src2 = reg->result.src2;
    RTLRegister * const src1_reg = &unit->regs[src1];
    RTLRegister * const src2_reg = &unit->regs[src2];

    uint64_t imm64;
    bool constant_is_src2;
    if (src1_reg->source == RTLREG_CONSTANT) {
        imm64 = reg_value_i64(src1_reg);
        constant_is_src2 = false;
    } else {
        ASSERT(src2_reg->source == RTLREG_CONSTANT);
        imm64 = reg_value_i64(src2_reg);
        constant_is_src2 = true;
    }
    if (reg->type != RTLTYPE_INT32
     && imm64 + UINT64_C(0x80000000) >= UINT64_C(0x100000000)) {
        return false;  // Doesn't fit in the immediate field.
    }
    int32_t imm = imm64;

    switch (opcode) {

      case RTLOP_ADD:
        reg->result.opcode = RTLOP_ADDI;
        break;

      case RTLOP_SUB:
        if (!constant_is_src2) {
            return false;  // "const - reg" can't be converted to a single op.
        } else if (reg->type != RTLTYPE_INT32
                   && imm64 == UINT64_C(-0x80000000)) {
            return false;  // Negated value would be out of range.
        }
        reg->result.opcode = RTLOP_ADDI;
        imm = -imm;
        break;

      case RTLOP_MUL:
        reg->result.opcode = RTLOP_MULI;
        break;

      case RTLOP_AND:
        reg->result.opcode = RTLOP_ANDI;
        break;

      case RTLOP_OR:
        reg->result.opcode = RTLOP_ORI;
        break;

      case RTLOP_XOR:
        reg->result.opcode = RTLOP_XORI;
        break;

      case RTLOP_SLL:
        if (!constant_is_src2) {
            return false;
        }
        reg->result.opcode = RTLOP_SLLI;
        break;

      case RTLOP_SRL:
        if (!constant_is_src2) {
            return false;
        }
        reg->result.opcode = RTLOP_SRLI;
        break;

      case RTLOP_SRA:
        if (!constant_is_src2) {
            return false;
        }
        reg->result.opcode = RTLOP_SRAI;
        break;

      case RTLOP_ROL:
        if (!constant_is_src2) {
            return false;
        }
        reg->result.opcode = RTLOP_RORI;
        /* We don't care if this is MIN_INT, since only the low bits matter. */
        imm = -imm;
        break;

      case RTLOP_ROR:
        if (!constant_is_src2) {
            return false;
        }
        reg->result.opcode = RTLOP_RORI;
        break;

      case RTLOP_SEQ:
        reg->result.opcode = RTLOP_SEQI;
        break;

      case RTLOP_SLTU:
        if (constant_is_src2) {
            reg->result.opcode = RTLOP_SLTUI;
        } else {
            reg->result.opcode = RTLOP_SGTUI;
        }
        break;

      case RTLOP_SLTS:
        if (constant_is_src2) {
            reg->result.opcode = RTLOP_SLTSI;
        } else {
            reg->result.opcode = RTLOP_SGTSI;
        }
        break;

      case RTLOP_SGTU:
        if (constant_is_src2) {
            reg->result.opcode = RTLOP_SGTUI;
        } else {
            reg->result.opcode = RTLOP_SLTUI;
        }
        break;

      case RTLOP_SGTS:
        if (constant_is_src2) {
            reg->result.opcode = RTLOP_SGTSI;
        } else {
            reg->result.opcode = RTLOP_SLTSI;
        }
        break;

      /* The remainder either never appear with RTLREG_RESULT, have only
       * one operand, or have no register-immediate equivalent. */
      case RTLOP_NOP:
      case RTLOP_SET_ALIAS:
      case RTLOP_GET_ALIAS:
      case RTLOP_MOVE:
      case RTLOP_SELECT:
      case RTLOP_SCAST:
      case RTLOP_ZCAST:
      case RTLOP_SEXT8:
      case RTLOP_SEXT16:
      case RTLOP_NEG:
      case RTLOP_MULHU:
      case RTLOP_MULHS:
      case RTLOP_DIVU:
      case RTLOP_DIVS:
      case RTLOP_MODU:
      case RTLOP_MODS:
      case RTLOP_NOT:
      case RTLOP_CLZ:
      case RTLOP_BSWAP:
      case RTLOP_BFEXT:
      case RTLOP_BFINS:
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
      case RTLOP_CALL:
      case RTLOP_CALL_TRANSPARENT:
      case RTLOP_RETURN:
      case RTLOP_ILLEGAL:
        return false;

    }  // switch ((RTLOpcode)reg->result.opcode)

    /* Anything that breaks out of the switch (instead of returning false)
     * is a successful conversion. */
    ASSERT(reg->result.opcode != opcode);
    reg->result.is_imm = true;
    reg->result.src1 = constant_is_src2 ? src1 : src2;
    reg->result.src_imm = imm;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * fold_one_register:  Attempt to perform constant folding on a register.
 * The register must be of type RTLREG_RESULT; all operands are assumed
 * to have been folded if possible.
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 *     reg: Register on which to perform constant folding.
 * [Return value]
 *     True if constant folding was performed, false if not.
 */
static inline void fold_one_register(RTLUnit * const unit,
                                     RTLRegister * const reg)
{
    ASSERT(unit);
    ASSERT(reg);
    ASSERT(reg->live);
    ASSERT(reg->source == RTLREG_RESULT);

    RTLInsn * const birth_insn = &unit->insns[reg->birth];

    /* Flag the register as not foldable by default. */
    reg->source = RTLREG_RESULT_NOFOLD;

    /* Convert SELECT with a constant condition operand to MOVE.  The MOVE
     * will be folded in turn if possible. */
    if (reg->result.opcode == RTLOP_SELECT) {
        const int src1 = reg->result.src1;
        const int src2 = reg->result.src2;
        const int src3 = reg->result.src3;
        if (unit->regs[src3].source != RTLREG_CONSTANT) {
            return;  // Can't fold anything if the condition isn't constant.
        }
        const bool condition = (unit->regs[reg->result.src3].value.i64 != 0);
        birth_insn->opcode = RTLOP_MOVE;
        if (!condition) {
            birth_insn->src1 = src2;
            reg->result.src1 = src2;
        }
        birth_insn->src2 = 0;
        birth_insn->src3 = 0;
        reg->result.opcode = RTLOP_MOVE;
        reg->result.src2 = 0;
        reg->result.src3 = 0;
#ifdef RTL_DEBUG_OPTIMIZE
        log_info(unit->handle, "Reduced r%d SELECT (always %s) to MOVE from"
                 " r%d at %d", (int)(reg - unit->regs),
                 condition ? "true" : "false", reg->result.src1, reg->birth);
#endif
        if (!condition && unit->regs[src1].death == reg->birth) {
            rollback_reg_death(unit, src1);
        }
        if (condition && unit->regs[src2].death == reg->birth) {
            rollback_reg_death(unit, src2);
        }
        if (unit->regs[src3].death == reg->birth) {
            rollback_reg_death(unit, src3);
        }
    }

    /* Fold the register to a constant, if possible.  If one of the two
     * operands is constant but the other is not, convert the instruction
     * to its equivalent register-immediate instruction if one exists. */
    const int src1 = reg->result.src1;
    const int src2 = reg->result.is_imm ? 0 : reg->result.src2;
    const bool src1_is_constant = (unit->regs[src1].source == RTLREG_CONSTANT);
    const bool src2_is_constant = (unit->regs[src2].source == RTLREG_CONSTANT);
    bool folded = false;
    if (src1_is_constant && (!src2 || src2_is_constant)) {
        reg->source = RTLREG_CONSTANT;
        reg->value.i64 = fold_constant(unit, reg);
        if (reg->type == RTLTYPE_INT32) {
            reg->value.i64 = (uint32_t)reg->value.i64;
        }
        birth_insn->opcode = RTLOP_LOAD_IMM;
        birth_insn->src1 = 0;
        birth_insn->src2 = 0;
        birth_insn->src_imm = reg->value.i64;
#ifdef RTL_DEBUG_OPTIMIZE
        char value_str[20];
        if (rtl_register_is_int(reg)) {
            format_int(value_str, sizeof(value_str),
                       reg->type, reg->value.i64);
        } else {
            snprintf_assert(value_str, sizeof(value_str),
                            "0x%"PRIX64, reg->value.i64);
        }
        log_info(unit->handle, "Folded r%d to constant value %s at %d",
                 (int)(reg - unit->regs), value_str, reg->birth);
#endif
        folded = true;
    } else if (src1_is_constant || (src2 && src2_is_constant)) {
        if (convert_to_regimm(unit, reg)) {
            ASSERT(reg->result.is_imm);
            birth_insn->opcode = reg->result.opcode;
            birth_insn->src1 = reg->result.src1;
            birth_insn->src2 = 0;
            birth_insn->src_imm = reg->result.src_imm;
#ifdef RTL_DEBUG_OPTIMIZE
            log_info(unit->handle, "Reduced r%d to register-immediate"
                     " operation (r%d, %d) at %d",
                     (int)(reg - unit->regs), reg->result.src1,
                     reg->result.src_imm, reg->birth);
#endif
            folded = true;
        }
    }

    /* If folding was successful and a folded operand died on this
     * instruction, roll its death point back to its previous use. */
    if (folded) {
        if (src1_is_constant && unit->regs[src1].death == reg->birth) {
            rollback_reg_death(unit, src1);
        }
        if (src2 && src2_is_constant && unit->regs[src2].death == reg->birth) {
            rollback_reg_death(unit, src2);
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_readonly_ptr:  Return a pointer to the address referenced by the
 * given register's load instruction if that address is constant and within
 * a region of guest memory marked as read-only, otherwise NULL.
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 *     reg: Register to check (assumed to be RTLREG_MEMORY).
 * [Return value]
 *     Pointer to the load source for the register, or NULL if the
 *     register does not load from read-only memory.
 */
static const void *get_readonly_ptr(RTLUnit * const unit,
                                    RTLRegister * const reg)
{
    ASSERT(unit);
    ASSERT(unit->handle);
    ASSERT(reg);
    ASSERT(reg->source == RTLREG_MEMORY);

    const RTLRegister * const base_reg = &unit->regs[reg->memory.base];
    if (base_reg->source != RTLREG_CONSTANT) {
        return NULL;
    }
    const uint32_t addr = (uint32_t)base_reg->value.i64 + reg->memory.offset;
    const uint32_t page = addr >> READONLY_PAGE_BITS;

    bool is_readonly;
    const binrec_t * const handle = unit->handle;
    if (handle->readonly_map[page>>2] & (2 << ((page & 3) * 2))) {
        is_readonly = true;
    } else if (handle->readonly_map[page>>2] & (1 << ((page & 3) * 2))) {
        const int index = lookup_partial_readonly_page(handle, page);
        ASSERT(index < lenof(handle->partial_readonly_pages));
        ASSERT(handle->partial_readonly_pages[index] == page);
        const uint32_t page_offset = addr & READONLY_PAGE_MASK;
        is_readonly = ((handle->partial_readonly_map[index][page_offset>>3]
                        & (1 << (page_offset & 7))) != 0);
    } else {
        is_readonly = false;
    }

    if (is_readonly) {
        return (void *)((uintptr_t)handle->setup.guest_memory_base + addr);
    } else {
        return NULL;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * fold_readonly_load:  Convert a load from read-only memory to an
 * equivalent load-immediate instruction.
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 *     reg: Register on which to perform constant folding.
 *     load_ptr: Pointer to source data for the load.
 */
static void fold_readonly_load(RTLUnit * const unit, RTLRegister * const reg,
                               const void *load_ptr)
{
    ASSERT(unit);
    ASSERT(reg);
    ASSERT(load_ptr);

    uint64_t value;
    if (reg->memory.size == 1) {
        value = *(const uint8_t *)load_ptr;
        if (reg->memory.is_signed) {
            value = (uint32_t)(int8_t)value;
        }
    } else if (reg->memory.size == 2) {
        value = *(const uint16_t *)load_ptr;
        if (reg->memory.byterev ^ unit->handle->host_little_endian) {
            value = bswap_le16(value);
        } else {
            value = bswap_be16(value);
        }
        if (reg->memory.is_signed) {
            value = (uint32_t)(int16_t)value;
        }
    } else {
        ASSERT(reg->memory.size == 0);
        switch ((RTLDataType)reg->type) {
          case RTLTYPE_INT32:
          case RTLTYPE_FLOAT32:
            value = *(const uint32_t *)load_ptr;
            if (reg->memory.byterev ^ unit->handle->host_little_endian) {
                value = bswap_le32(value);
            } else {
                value = bswap_be32(value);
            }
            break;
          case RTLTYPE_INT64:
          case RTLTYPE_ADDRESS:
          case RTLTYPE_FLOAT64:
            value = *(const uint64_t *)load_ptr;
            if (reg->memory.byterev ^ unit->handle->host_little_endian) {
                value = bswap_le64(value);
            } else {
                value = bswap_be64(value);
            }
            break;
          default:
            return;  // Vector types have no LOAD_IMM equivalent.
        }
    }

    const int base = reg->memory.base;

#ifdef RTL_DEBUG_OPTIMIZE
    char value_str[20];
    if (rtl_register_is_int(reg)) {
        format_int(value_str, sizeof(value_str), reg->type, value);
    } else {
        snprintf_assert(value_str, sizeof(value_str), "0x%"PRIX64, value);
    }
    const uint32_t addr =
        ((uint32_t)unit->regs[base].value.i64 + reg->memory.offset);
    log_info(unit->handle, "r%d loads constant value %s from 0x%X at %d",
             (int)(reg - unit->regs), value_str, addr, reg->birth);
#endif

    reg->source = RTLREG_CONSTANT;
    reg->value.i64 = value;
    RTLInsn *reg_insn = &unit->insns[reg->birth];
    reg_insn->opcode = RTLOP_LOAD_IMM;
    reg_insn->src_imm = value;

    if (unit->regs[base].death == reg->birth) {
        rollback_reg_death(unit, base);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * is_alias_store_visible:  Return whether a store to the given alias in
 * the given basic block is visible to any other alias reference.  If the
 * store is visible at an exit from the unit, it is treated as visible if
 * the alias has bound storage and not visible otherwise.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     alias_info: Alias reference data array.
 *     alias: Index in unit->aliases[] of alias to check.
 *     block_index: Index in unit->blocks[] of block to start search at.
 * [Return value]
 *     True if the store is visible, false if not.
 */
static bool is_alias_store_visible(RTLUnit * const unit, AliasRef **alias_info,
                                   const int alias, const int block_index)
{
    const RTLBlock * const block = &unit->blocks[block_index];
    if (block->exits[0] < 0) {
        return unit->aliases[alias].base != 0;
    } else {
        /* Rather than simply iterating over the exit list, we first
         * recurse on the second exit if it exists, then recurse on the
         * first exit; this allows the latter recursion to be a tail call. */
        STATIC_ASSERT(lenof(block->exits) == 2, "RTLBlock must have 2 exits");

        if (block->exits[1] >= 0 && !unit->block_seen[block->exits[1]]) {
            const int exit = block->exits[1];
            unit->block_seen[exit] = true;
            if (alias_info[exit][alias].has_get) {
                return true;
            } else if (!alias_info[exit][alias].has_set
                    && is_alias_store_visible(unit, alias_info, alias, exit)) {
                return true;
            }
        }

        const int exit = block->exits[0];
        if (unit->block_seen[exit]) {
            return false;
        } else {
            unit->block_seen[exit] = true;
            if (alias_info[exit][alias].has_get) {
                return true;
            } else if (alias_info[exit][alias].has_set) {
                return false;
            } else {
                return is_alias_store_visible(unit, alias_info, alias, exit);
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * kill_alias_store:  Kill (change to NOP) the given SET_ALIAS instruction.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     insn_index: Index of instruction to kill.
 */
static void kill_alias_store(RTLUnit * const unit, const int insn_index)
{
    ASSERT(unit);
    ASSERT(unit->insns[insn_index].opcode == RTLOP_SET_ALIAS);

    RTLInsn *insn = &unit->insns[insn_index];
#ifdef RTL_DEBUG_OPTIMIZE
    log_info(unit->handle, "Dropping dead store to a%d at %d",
             insn->alias, insn_index);
#endif
    const int src1 = insn->src1;
    insn->opcode = RTLOP_NOP;
    insn->src1 = 0;
    insn->src_imm = 0;
    if (unit->regs[src1].death == insn_index) {
        rollback_reg_death(unit, src1);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * maybe_kill_store:  Kill (change to NOP) the instruction that sets the
 * given register if the instruction is a killable one.  Recurseive helper
 * for rtl_opt_drop_dead_stores().
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg: Register to possibly kill store for.
 */
static void maybe_kill_store(RTLUnit * const unit, RTLRegister * const reg)
{
    ASSERT(unit);
    ASSERT(reg);
    ASSERT(reg->death == reg->birth);

    const int insn_index = reg->birth;
    RTLInsn * const insn = &unit->insns[insn_index];

    /* Don't drop instructions with side effects! */
    if (insn->opcode == RTLOP_ATOMIC_INC || insn->opcode == RTLOP_CMPXCHG
     || insn->opcode == RTLOP_CALL || insn->opcode == RTLOP_CALL_TRANSPARENT) {
        return;
    }

#ifdef RTL_DEBUG_OPTIMIZE
    log_info(unit->handle, "Dropping dead store to r%d at %d",
             insn->dest, insn_index);
#endif

    if (insn->src1) {
        RTLRegister * const src1_reg = &unit->regs[insn->src1];
        if (src1_reg->death == insn_index) {
            if (rollback_reg_death(unit, insn->src1)) {
                maybe_kill_store(unit, src1_reg);
            }
        }
    }
    if (insn->src2) {
        RTLRegister * const src2_reg = &unit->regs[insn->src2];
        if (src2_reg->death == insn_index) {
            if (rollback_reg_death(unit, insn->src2)) {
                maybe_kill_store(unit, src2_reg);
            }
        }
    }
    if (insn->opcode == RTLOP_SELECT) {
        ASSERT(insn->src3);
        RTLRegister * const src3_reg = &unit->regs[insn->src3];
        if (src3_reg->death == insn_index) {
            if (rollback_reg_death(unit, insn->src3)) {
                maybe_kill_store(unit, src3_reg);
            }
        }
    }

    insn->opcode = RTLOP_NOP;
    insn->dest = 0;
    insn->src1 = 0;
    insn->src2 = 0;
    insn->src_imm = 0;
}

/*-----------------------------------------------------------------------*/

/**
 * thread_branch:  Thread the given branch instruction through to its
 * final destination.  Helper function for rtl_opt_thread_branches().
 *
 * [Parameters]
 *     unit: RTL unit.
 *     block_index: Index in unit->blocks[] of block containing branch.
 *     branch: Pointer to branch instruction.
 */
static void thread_branch(RTLUnit * const unit, int block_index,
                          RTLInsn * const branch)
{
    memset(unit->block_seen, 0, unit->num_blocks * sizeof(*unit->block_seen));
    unit->block_seen[block_index] = 1;

    int new_label = 0, new_target;
    const int target_block = unit->label_blockmap[branch->label];
    ASSERT(target_block >= 0 && target_block < unit->num_blocks);
    for (uint32_t i = unit->blocks[target_block].first_insn;
         i < unit->num_insns; i++)
    {
        const RTLInsn * const insn = &unit->insns[i];
        if (insn->opcode == RTLOP_GOTO
         || (insn->opcode == branch->opcode && insn->src1 == branch->src1)) {
            const int label = insn->label;
            new_target = unit->label_blockmap[label];
            ASSERT(new_target >= 0 && new_target < unit->num_blocks);
            if (UNLIKELY(unit->block_seen[new_target])) {
#ifdef RTL_DEBUG_OPTIMIZE
                log_info(unit->handle, "Branch at %d has a cycle, not"
                         " threading", (int)(branch - unit->insns));
#endif
                new_label = 0;
                break;
            }
            new_label = label;
            unit->block_seen[new_target] = 1;
            i = unit->blocks[new_target].first_insn - 1;
        } else if (insn->opcode != RTLOP_NOP && insn->opcode != RTLOP_LABEL) {
            break;
        }
    }

    if (new_label) {
        /* Update the instruction and flow graph.  If this is a conditional
         * branch to the next instruction, we'll be replacing the
         * fall-through edge along with the branch edge here; but in that
         * case, the conditional branch is a NOP anyway, so we've
         * effectively just removed a dead block from the flow graph. */
        RTLBlock * const block = &unit->blocks[block_index];
        const int exit_index = (block->exits[1] == target_block ? 1 : 0);
        rtl_block_remove_edge(unit, block_index, exit_index);
        if (UNLIKELY(!rtl_block_add_edge(unit, block_index, new_target))) {
            ASSERT(rtl_block_add_edge(unit, block_index, target_block));
            log_warning(unit->handle, "Out of memory while threading branch at"
                     " %d", (int)(branch - unit->insns));
        } else {
#ifdef RTL_DEBUG_OPTIMIZE
            log_info(unit->handle, "Threading branch at %d through to final"
                     " target L%d", (int)(branch - unit->insns), new_label);
#endif
            branch->label = new_label;
        }
    }
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
    /* As in is_alias_store_visible(), use tail recursion for the first
     * exit edge. */
    for (int i = 1; i < lenof(block->exits) && block->exits[i] >= 0; i++) {
        if (!unit->block_seen[block->exits[i]]) {
            visit_block(unit, block->exits[i]);
        }
    }
    if (block->exits[0] >= 0 && !unit->block_seen[block->exits[0]]) {
        visit_block(unit, block->exits[0]);
    }
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

void rtl_opt_alias_data_flow(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);
    ASSERT(unit->aliases);
    ASSERT(unit->block_seen);

    const int alias_info_size =
        sizeof(AliasRef) * unit->num_blocks * unit->next_alias;
    const int ptr_array_size = sizeof(AliasRef *) * unit->num_blocks;
    /* "char *" since "char" is guaranteed to be a basic memory unit (byte). */
    char *alias_info_buf = rtl_malloc(unit, alias_info_size + ptr_array_size);
    if (UNLIKELY(!alias_info_buf)) {
        log_warning(unit->handle, "No memory for alias tracking, skipping"
                    " data flow analysis");
        return;
    }
    memset(alias_info_buf + ptr_array_size, 0, alias_info_size);
    AliasRef **alias_info = (AliasRef **)alias_info_buf;
    for (int i = 0; i < unit->num_blocks; i++) {
        alias_info[i] =
            (AliasRef *)(alias_info_buf + ptr_array_size
                         + (sizeof(AliasRef) * i * unit->next_alias));
    }

    /* Look up alias references in each block.  We only record the last
     * SET_ALIAS instruction in each block since that's the only one that
     * matters for data flow analysis; any other SET_ALIAS without a
     * following GET_ALIAS (or CALL/CALL_TRANSPARENT, if the alias has
     * bound storage) is dead, so we NOP it out. */
    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        const RTLBlock *block = &unit->blocks[block_index];
        AliasRef *alias_refs = alias_info[block_index];
        for (int insn_index = block->first_insn;
             insn_index <= block->last_insn; insn_index++)
        {
            RTLInsn *insn = &unit->insns[insn_index];
            const int alias = insn->alias;
            AliasRef *alias_ref = &alias_refs[alias];
            switch (insn->opcode) {
              case RTLOP_SET_ALIAS:
                if (alias_ref->has_set && !alias_ref->set_used) {
                    kill_alias_store(unit, alias_ref->set_insn);
                }
                alias_ref->has_set = true;
                alias_ref->set_used = false;
                alias_ref->set_insn = insn_index;
                break;
              case RTLOP_GET_ALIAS:
                if (alias_ref->has_set) {
                    alias_ref->set_used = true;
                } else {
                    alias_ref->has_get = true;
                }
                break;
              case RTLOP_CALL:
              case RTLOP_CALL_TRANSPARENT:
                /* A call-type instruction is effectively a reference to
                 * any previously set alias with bound storage, since the
                 * value has to be flushed to storage before the call. */
                for (int i = 1; i < unit->next_alias; i++) {
                    if (unit->aliases[i].base && alias_refs[i].has_set) {
                        alias_refs[i].set_used = true;
                    }
                }
                break;
              default:
                break;
            }
        }
    }

    /* Trace the visibility of each recorded SET_ALIAS instruction.  If
     * there are no code paths on which the store is visible (and for
     * aliases with bound storage, if the store is not visible at any exit
     * from the unit), kill it. */
    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        AliasRef *alias_refs = alias_info[block_index];
        for (int alias = 1; alias < unit->next_alias; alias++) {
            AliasRef *alias_ref = &alias_refs[alias];
            if (alias_ref->has_set && !alias_ref->set_used) {
                memset(unit->block_seen, 0,
                       sizeof(*unit->block_seen) * unit->num_blocks);
                if (!is_alias_store_visible(unit, alias_info, alias,
                                            block_index)) {
                    kill_alias_store(unit, alias_ref->set_insn);
                }
            }
        }
    }

    rtl_free(unit, alias_info_buf);
}

/*-----------------------------------------------------------------------*/

void rtl_opt_decondition(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);

    /* A conditional branch always ends a basic block and has two targets,
     * so we only need to check the last instruction of each block with two
     * exit edges. */

    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        RTLBlock * const block = &unit->blocks[block_index];
        if (block->exits[1] >= 0) {
            /* If the block has multiple exits, the last one must be a
             * conditional branch. */
            ASSERT(block->last_insn >= block->first_insn);
            RTLInsn * const insn = &unit->insns[block->last_insn];
            const RTLOpcode opcode = insn->opcode;
            ASSERT(opcode == RTLOP_GOTO_IF_Z || opcode == RTLOP_GOTO_IF_NZ);

            /* See if the branch's condition is constant. */
            const int reg_index = insn->src1;
            RTLRegister * const condition_reg = &unit->regs[reg_index];
            if (condition_reg->source == RTLREG_CONSTANT) {
                const uint64_t condition = condition_reg->value.i64;
                const int fallthrough_index =
                    (block->exits[0] == block_index+1) ? 0 : 1;

                /* Convert the branch to either an unconditional GOTO or
                 * a NOP depending on the condition value. */
                if ((opcode == RTLOP_GOTO_IF_Z && condition == 0)
                 || (opcode == RTLOP_GOTO_IF_NZ && condition != 0)) {
                    /* Branch always taken: convert to GOTO. */
#ifdef RTL_DEBUG_OPTIMIZE
                    log_info(unit->handle, "Branch at %u always taken,"
                             " converting to GOTO and dropping edge %u->%u",
                             block->last_insn, block_index,
                             block->exits[fallthrough_index]);
#endif
                    insn->opcode = RTLOP_GOTO;
                    insn->src1 = 0;
                    rtl_block_remove_edge(unit, block_index,
                                          fallthrough_index);
                } else {
                    /* Branch never taken: convert to NOP. */
#ifdef RTL_DEBUG_OPTIMIZE
                    log_info(unit->handle, "Branch at %u never taken,"
                             " converting to NOP and dropping edge %u->%u",
                             block->last_insn, block_index,
                             block->exits[fallthrough_index ^ 1]);
#endif
                    insn->opcode = RTLOP_NOP;
                    insn->src1 = 0;
                    insn->src_imm = 0;
                    rtl_block_remove_edge(unit, block_index,
                                          fallthrough_index ^ 1);
                }

                /* If the condition register dies here, roll its death
                 * back to its previous use. */
                if (condition_reg->death == block->last_insn) {
                    rollback_reg_death(unit, reg_index);
                }
            }  // if (condition_reg->source == RTLREG_CONSTANT)
        }  // if (block->exits[1] != -1)
    }  // for (block_index)
}

/*-----------------------------------------------------------------------*/

void rtl_opt_drop_dead_blocks(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);
    ASSERT(unit->block_seen);

    /* Recursively visit all blocks reachable from the start block. */
    memset(unit->block_seen, 0, unit->num_blocks * sizeof(*unit->block_seen));
    visit_block(unit, 0);

    /* Remove all blocks which are not reachable. */
    for (int i = 0; i >= 0; i = unit->blocks[i].next_block) {
        RTLBlock * const block = &unit->blocks[i];
        if (!unit->block_seen[i]) {
#ifdef RTL_DEBUG_OPTIMIZE
            log_info(unit->handle, "Dropping dead block %d (%d-%d)",
                     i, block->first_insn, block->last_insn);
#endif
            ASSERT(block->prev_block >= 0);
            unit->blocks[block->prev_block].next_block = block->next_block;
            if (block->next_block >= 0) {
                unit->blocks[block->next_block].prev_block = block->prev_block;
            }
            /* Remove all exiting edges (required since some edges may go
             * to non-dead blocks).  Note that we can just iterate on
             * block->exits[0] since rtl_block_remove_edge() will shift
             * the array downward after removing the edge. */
            while (block->exits[0] >= 0) {
                rtl_block_remove_edge(unit, i, 0);
            }
        }
    }

    ASSERT(unit->num_blocks <= unit->block_seen_size);
}

/*-----------------------------------------------------------------------*/

void rtl_opt_drop_dead_branches(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);
    ASSERT(unit->label_blockmap);

    for (int i = 0; i >= 0; i = unit->blocks[i].next_block) {
        RTLBlock * const block = &unit->blocks[i];
        if (block->last_insn >= block->first_insn) {
            RTLInsn * const insn = &unit->insns[block->last_insn];
            const RTLOpcode opcode = insn->opcode;
            if ((opcode == RTLOP_GOTO
              || opcode == RTLOP_GOTO_IF_Z
              || opcode == RTLOP_GOTO_IF_NZ)
             && unit->label_blockmap[insn->label] == block->next_block) {
#ifdef RTL_DEBUG_OPTIMIZE
                log_info(unit->handle, "Dropping branch at %d to next"
                         " insn", block->last_insn);
#endif
                if (opcode == RTLOP_GOTO_IF_Z || opcode == RTLOP_GOTO_IF_NZ) {
                    if (unit->regs[insn->src1].death == block->last_insn) {
                        rollback_reg_death(unit, insn->src1);
                    }
                }
                insn->opcode = RTLOP_NOP;
                insn->src1 = 0;
                insn->src2 = 0;
                insn->src_imm = 0;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

void rtl_opt_drop_dead_stores(RTLUnit *unit)
{
    for (int reg_index = 1; reg_index < unit->next_reg; reg_index++) {
        RTLRegister *reg = &unit->regs[reg_index];
        if (reg->death == reg->birth) {
            maybe_kill_store(unit, reg);
        }
    }
}

/*-----------------------------------------------------------------------*/

void rtl_opt_fold_constants(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->regs);

    for (uint32_t insn_index = 0; insn_index < unit->num_insns; insn_index++) {
        const RTLInsn *insn = &unit->insns[insn_index];
        if (insn->dest) {
            RTLRegister * const reg = &unit->regs[insn->dest];
            if (reg->source == RTLREG_RESULT) {
                fold_one_register(unit, reg);
            } else if (reg->source == RTLREG_MEMORY) {
                const void *readonly_ptr = get_readonly_ptr(unit, reg);
                if (readonly_ptr) {
                    fold_readonly_load(unit, reg, readonly_ptr);
                }
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

void rtl_opt_thread_branches(RTLUnit *unit)
{
    ASSERT(unit);
    ASSERT(unit->insns);
    ASSERT(unit->blocks);
    ASSERT(unit->regs);
    ASSERT(unit->label_blockmap);
    ASSERT(unit->block_seen);

    for (int i = 0; i >= 0; i = unit->blocks[i].next_block) {
        RTLBlock * const block = &unit->blocks[i];
        if (block->last_insn >= block->first_insn) {
            RTLInsn * const insn = &unit->insns[block->last_insn];
            if ((insn->opcode == RTLOP_GOTO
              || insn->opcode == RTLOP_GOTO_IF_Z
              || insn->opcode == RTLOP_GOTO_IF_NZ)) {
                thread_branch(unit, i, insn);
            }
        }
    }

    ASSERT(unit->num_blocks <= unit->block_seen_size);
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
