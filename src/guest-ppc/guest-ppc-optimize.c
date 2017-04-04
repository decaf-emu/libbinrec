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
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl-internal.h"

#include <inttypes.h>

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
 * kill_alias_store:  Eliminate the given SET_ALIAS instruction, and copy
 * the instruction which set the source register's value into *set_insn_ret.
 * If kill_value is true and the source register is unused other than by
 * that instruction, eliminate it as well.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of instruction to eliminate (must be SET_ALIAS).
 *     kill_value: True to kill the instruction which set the alias value
 *         (if possible), false to leave it alone.
 *     set_insn_ret: Copy of the instruction which set the alias value.
 *         May be NULL, in which case an eliminated value-setting
 *         instruction is discarded.
 * [Return value]
 *     True if the value-setting instruction was eliminated, false if not.
 */
static bool kill_alias_store(
    GuestPPCContext *ctx, int insn_index, bool kill_value,
    struct RTLInsn *set_insn_ret)
{
    ASSERT(ctx);
    ASSERT(insn_index >= 0);
    ASSERT((unsigned int)insn_index < ctx->unit->num_insns);
    ASSERT(ctx->unit->insns[insn_index].opcode == RTLOP_SET_ALIAS);

    RTLUnit * const unit = ctx->unit;

    const int reg = unit->insns[insn_index].src1;
    rtl_opt_kill_insn(unit, insn_index, false, false);

    const int birth = unit->regs[reg].birth;
    if (set_insn_ret) {
        *set_insn_ret = unit->insns[birth];
    }

    if (kill_value && is_reg_killable(unit, reg, insn_index)) {
        rtl_opt_kill_insn(unit, birth, false, false);
        return true;
    } else {
        return false;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * kill_cr_stores:  Kill stores to the CR bits given by bits_to_kill.
 * If crb_reg_ret and crb_insn_ret are non-NULL, save the RTL register for
 * each bit and the instruction which sets it in the corresponding entries
 * of those arrays.  Helper function for guest_ppc_trim_cr_stores().
 *
 * [Parameters]
 *     ctx: Translation context.
 *     BO: BO field of the branch instruction (0x14 for an unconditional
 *         branch).
 *     BI: BI field of the branch instruction (ignored for unconditional
 *         branches).
 *     bits_to_kill: Bitmask of CR bits to kill (LSB = CR bit 0).
 *     crb_reg: Pointer to 32-element array to receive the RTL register
 *         containing the value for each killed bit, or NULL if not needed.
 *     crb_insn: Pointer to 32-element array to receive the RTL instruction
 *         which sets the value for each killed bit, or NULL if not needed.
 */
static inline void kill_cr_stores(
    GuestPPCContext *ctx, int BO, int BI, uint32_t bits_to_kill,
    uint16_t *crb_reg, RTLInsn *crb_insn)
{
    RTLUnit * const unit = ctx->unit;

    while (bits_to_kill) {
        const int bit = ctz32(bits_to_kill);
        bits_to_kill ^= 1 << bit;
        const int insn_index = ctx->last_set.crb[bit];
        ASSERT(insn_index >= 0);
        ctx->last_set.crb[bit] = -1;
        if (crb_reg) {
            crb_reg[bit] = unit->insns[insn_index].src1;
        }
        /* Careful not to kill a value which is the branch condition! */
        const bool preserve_value = (!(BO & 0x10) && bit == BI);
        if (!kill_alias_store(ctx, insn_index, !preserve_value,
                              crb_insn ? &crb_insn[bit] : NULL)) {
            if (crb_insn) {
                crb_insn[bit].opcode = 0;
            }
        }
    }
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

bool guest_ppc_detect_fcfi_emul(
    GuestPPCContext *ctx, uint32_t address, int frA, int frB,
    int *src_ret, bool *is_signed_ret)
{
    RTLUnit * const unit = ctx->unit;

    /* Check that the operands have already been seen in this block and are
     * of the proper type. */
    if (!ctx->live.fpr[frA] || !ctx->live.fpr[frB]) {
        return false;
    }
    const RTLRegister *frA_reg = &unit->regs[ctx->live.fpr[frA]];
    const RTLRegister *frB_reg = &unit->regs[ctx->live.fpr[frB]];
    if (frA_reg->type != RTLTYPE_FLOAT64 || frB_reg->type != RTLTYPE_FLOAT64) {
        return false;
    }

    /* Check that the second operand is a constant of the proper value for
     * a signed or unsigned conversion, and record which one it is. */
    bool is_signed;
    if (frB_reg->source != RTLREG_MEMORY) {
        return false;
    }
    /* There are no byte-reversed floating-point load instructions (and
     * floating-point values are never loaded directly from the PSB), so
     * the byte order of the load should always be big-endian. */
    ASSERT(frB_reg->memory.byterev == ctx->handle->host_little_endian);
    uint32_t frB_address;
    const RTLRegister *frB_base_reg = &unit->regs[frB_reg->memory.base];
    if (frB_base_reg->source == RTLREG_FUNC_ARG) {
        /* Floating-point values are never loaded directly from the PSB,
         * so this must be a guest-absolute memory reference. */
        ASSERT(frB_base_reg->arg_index == 1);
        frB_address = frB_reg->memory.offset;
    } else {
        /* If the base register is not the guest memory base (FUNC_ARG 1),
         * it must be RTLREG_RESULT for a non-absolute guest load operation,
         * in which case all of the following are always true.  (We omit
         * them from the if condition since they're not testable, but we
         * assert just in case future changes to the translator logic break
         * those assumptions.) */
        ASSERT(frB_base_reg->source == RTLREG_RESULT);
        ASSERT(frB_base_reg->result.opcode == RTLOP_ADD);
        const RTLRegister *frB_base_src1_reg =
            &unit->regs[frB_base_reg->result.src1];
        const RTLRegister *frB_base_src2_reg =
            &unit->regs[frB_base_reg->result.src2];
        ASSERT(frB_base_src1_reg->source == RTLREG_FUNC_ARG);
        ASSERT(frB_base_src1_reg->arg_index == 1);
        ASSERT(frB_base_src2_reg->source == RTLREG_RESULT);
        ASSERT(frB_base_src2_reg->result.opcode == RTLOP_ZCAST);
        if (unit->regs[frB_base_src2_reg->result.src1].source == RTLREG_CONSTANT) {
            frB_address =
                (uint32_t)unit->regs[frB_base_src2_reg->result.src1].value.i64
                + frB_reg->memory.offset;
        } else {
            return false;
        }
    }
    if (!is_address_readonly(ctx->handle, frB_address)) {
        return false;
    }
    const uint8_t *frB_ptr =
        (const uint8_t *)ctx->handle->setup.guest_memory_base + frB_address;
    uint64_t frB_value_buf;
    memcpy(&frB_value_buf, frB_ptr, 8);
    const uint64_t frB_value = bswap_be64(frB_value_buf);
    if (frB_value == UINT64_C(0x4330000000000000)) {
        is_signed = false;
    } else if (frB_value == UINT64_C(0x4330000080000000)) {
        is_signed = true;
    } else {
        return false;
    }

    /* Check that the first operand is a value loaded from the local stack
     * frame.  We don't necessarily know the size of the stack frame, so we
     * just assume that any positive offset from the stack pointer (beyond
     * the ABI-required backchain and LR save area) is a reference to the
     * current frame. */
    int frA_offset = 0;
    if (frA_reg->source != RTLREG_MEMORY) {
        #ifdef RTL_DEBUG_OPTIMIZE
            /* If frB is a proper constant, the code is probably intended
             * as fcfi emulation, so at this point it makes sense to warn
             * about detection failures. */
            log_info(ctx->handle, "Failed to optimize possible fcfi at 0x%X:"
                     " frA is not the result of an lfd", address);
        #endif
        return false;
    }
    ASSERT(frA_reg->memory.byterev == ctx->handle->host_little_endian);
    const RTLRegister *frA_base_reg = &unit->regs[frA_reg->memory.base];
    if (frA_base_reg->source == RTLREG_RESULT) {
        /* These are always true for non-absolute loads. */
        ASSERT(frA_base_reg->result.opcode == RTLOP_ADD);
        ASSERT(unit->regs[frA_base_reg->result.src1].source == RTLREG_FUNC_ARG);
        ASSERT(unit->regs[frA_base_reg->result.src1].arg_index == 1);
        ASSERT(unit->regs[frA_base_reg->result.src2].source == RTLREG_RESULT);
        ASSERT(unit->regs[frA_base_reg->result.src2].result.opcode == RTLOP_ZCAST);
        if (unit->regs[frA_base_reg->result.src2].result.src1 == ctx->live.gpr[1]
         && frA_reg->memory.offset >= 8) {
            frA_offset = frA_reg->memory.offset;
        }
    }
    if (frA_offset == 0) {
        #ifdef RTL_DEBUG_OPTIMIZE
            log_info(ctx->handle, "Failed to optimize possible fcfi at 0x%X:"
                     " frA is not loaded from the local stack frame", address);
        #endif
        return false;
    }

    /* Look for two word stores to the stack frame at the proper offsets
     * to generate the 64-bit float value loaded into frA, and save the
     * RTL register which serves as the conversion input.  We only search
     * as far back as the beginning of the current basic block, which
     * should be sufficient for typical compiler-generated code. */
    const RTLOpcode store_op =
        ctx->handle->host_little_endian ? RTLOP_STORE_BR : RTLOP_STORE;
    bool found_hi = false, found_lo = false;
    int src = 0;
    for (int i = frA_reg->birth - 1; i >= ctx->cur_block_rtl_start; i--) {
        const RTLInsn *insn = &unit->insns[i];
        if (insn->opcode == RTLOP_STORE
         || insn->opcode == RTLOP_STORE_I8
         || insn->opcode == RTLOP_STORE_I16
         || insn->opcode == RTLOP_STORE_BR
         || insn->opcode == RTLOP_STORE_I16_BR) {
            const RTLRegister *base_reg = &unit->regs[insn->src1];
            if (base_reg->source != RTLREG_RESULT) {
                continue;  // Ignore absolute or PSB stores.
            }
            /* These are always true for non-absolute stores. */
            ASSERT(base_reg->result.opcode == RTLOP_ADD);
            ASSERT(unit->regs[base_reg->result.src1].source == RTLREG_FUNC_ARG);
            ASSERT(unit->regs[base_reg->result.src1].arg_index == 1);
            ASSERT(unit->regs[base_reg->result.src2].source == RTLREG_RESULT);
            ASSERT(unit->regs[base_reg->result.src2].result.opcode == RTLOP_ZCAST);
            if (unit->regs[base_reg->result.src2].result.src1 == ctx->live.gpr[1]
             && insn->offset >= frA_offset
             && insn->offset < frA_offset + 8) {
                if (insn->opcode != store_op) {
                    #ifdef RTL_DEBUG_OPTIMIZE
                        log_info(ctx->handle, "Failed to optimize possible"
                                 " fcfi at 0x%X: buffer clobbered by non-stw"
                                 " store", address);
                    #endif
                    return false;
                }
                const RTLRegister *value_reg = &unit->regs[insn->src2];
                if (insn->offset == frA_offset) {
                    /* High word must have the value 0x43300000. */
                    if (value_reg->source != RTLREG_CONSTANT) {
                        #ifdef RTL_DEBUG_OPTIMIZE
                            log_info(ctx->handle, "Failed to optimize possible"
                                     " fcfi at 0x%X: high word store is not"
                                     " constant", address);
                        #endif
                        return false;
                    }
                    if (value_reg->value.i64 != 0x43300000) {
                        #ifdef RTL_DEBUG_OPTIMIZE
                            log_info(ctx->handle, "Failed to optimize possible"
                                     " fcfi at 0x%X: high word store has the"
                                     " wrong value (0x%"PRIX64")", address,
                                     value_reg->value.i64);
                        #endif
                        return false;
                    }
                    found_hi = true;
                } else if (insn->offset == frA_offset + 4) {
                    /* For an unsigned conversion, the value stored in the
                     * low word is the value to convert.  For a signed
                     * conversion, the stored value is the value to convert
                     * with the high (sign) bit inverted, and we should see
                     * an xoris ...,0x8000 operation immediately preceding
                     * the store. */
                    if (is_signed) {
                        if (value_reg->source != RTLREG_RESULT
                         || value_reg->result.opcode != RTLOP_XORI
                         || value_reg->result.src_imm != (int32_t)0x80000000) {
                            #ifdef RTL_DEBUG_OPTIMIZE
                                log_info(ctx->handle,
                                         "Failed to optimize possible signed"
                                         " fcfi at 0x%X: low word store is not"
                                         " the result of xoris ...,0x8000",
                                         address);
                            #endif
                            return false;
                        }
                        src = value_reg->result.src1;
                    } else {
                        src = insn->src2;
                    }
                    found_lo = true;
                } else {
                    #ifdef RTL_DEBUG_OPTIMIZE
                        log_info(ctx->handle, "Failed to optimize possible"
                                 " fcfi at 0x%X: buffer clobbered by"
                                 " unaligned stw", address);
                    #endif
                    return false;
                }
                if (found_hi && found_lo) {
                    break;
                }
            }
        }
    }
    if (!found_hi || !found_lo) {
        #ifdef RTL_DEBUG_OPTIMIZE
            log_info(ctx->handle, "Failed to optimize possible fcfi at 0x%X:"
                     " stores to lfd buffer not found", address);
        #endif
        return false;
    }

    ASSERT(src != 0);
    *src_ret = src;
    *is_signed_ret = is_signed;
    return true;
}

/*-----------------------------------------------------------------------*/

void guest_ppc_trim_cr_stores(
    GuestPPCContext *ctx, int BO, int BI, uint32_t *crb_store_branch_ret,
    uint32_t *crb_store_next_ret, uint16_t *crb_reg_ret,
    RTLInsn *crb_insn_ret)
{
    ASSERT(ctx);

    const bool is_conditional = ((BO & 0x14) != 0x14);
    const int branch_block = ctx->blocks[ctx->current_block].branch_block;
    const int next_block = ctx->blocks[ctx->current_block].next_block;

    if (is_conditional) {
        ASSERT(crb_store_branch_ret);
        ASSERT(crb_store_next_ret);
        ASSERT(crb_reg_ret);
        ASSERT(crb_insn_ret);
    }

    /*
     * Note that this function does not attempt to handle the case where
     * multiple CR bits are set from the same RTL register.  Currently,
     * the only case in which this can happen is with stwcx., and in that
     * case one of the associated CR bits (CR0[eq]) is always flushed, so
     * the associated RTL register will never be killed.  If this was not
     * the case, we would need additional logic to detect when a register
     * was used by multiple bits and either leave it alive or insert it
     * only once when moving it to the appropriate code path.
     */

    /* Collect the set of bits which are both dirty and killable.  This
     * excludes registers which have already been flushed, since we can't
     * do anything about them. */
    uint32_t crb_dirty = 0;
    uint32_t dirty_temp = ctx->crb_dirty;
    while (dirty_temp) {
        const int bit = ctz32(dirty_temp);
        dirty_temp ^= 1 << bit;
        if (ctx->last_set.crb[bit] >= 0) {
            crb_dirty |= 1 << bit;
        }
    }

    /* First eliminate any stores which are dead on both taken and
     * not-taken paths.  For an unconditional branch, this will kill
     * all dead stores and the second half of the logic will be skipped. */
    const uint32_t crb_dead_branch =
        (branch_block < 0 ? 0 :
             ctx->blocks[branch_block].crb_changed_recursive & crb_dirty);
    const uint32_t crb_dead_next =
        (!is_conditional ? crb_dead_branch :
         next_block < 0 ? 0 :
             ctx->blocks[next_block].crb_changed_recursive & crb_dirty);
    const uint32_t crb_to_kill = crb_dead_branch & crb_dead_next;
    const uint32_t crb_store_branch = crb_dead_next & ~crb_to_kill;
    const uint32_t crb_store_next = crb_dead_branch & ~crb_to_kill;
    ASSERT((crb_store_branch & crb_store_next) == 0);
    /* These stores are unconditionally dead, so we don't need to save
     * the associated register or value-setting instruction. */
    kill_cr_stores(ctx, BO, BI, crb_to_kill, NULL, NULL);

    /* If crb_dead_branch or crb_dead_next is nonzero at this point, we
     * want to store those bits only on the code path where they are not
     * dead, so we kill the original SET_ALIAS instructions and (if
     * possible) the instructions which set the corresponding RTL register,
     * then re-add them at the branch or fall-through point. */
    const uint32_t crb_to_save = crb_store_branch | crb_store_next;
    ASSERT(!(crb_to_kill & crb_to_save));
    kill_cr_stores(ctx, BO, BI, crb_to_save, crb_reg_ret, crb_insn_ret);

    if (crb_store_branch_ret) {
        *crb_store_branch_ret = crb_store_branch;
    }
    if (crb_store_next_ret) {
        *crb_store_next_ret = crb_store_next;
    }
}

/*************************************************************************/
/*************************************************************************/
