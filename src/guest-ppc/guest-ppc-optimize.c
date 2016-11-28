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

void guest_ppc_trim_cr_stores(
    GuestPPCContext *ctx, int BO, int BI, uint32_t *crb_store_branch_ret,
    uint32_t *crb_store_next_ret, uint16_t *crb_reg_ret,
    RTLInsn *crb_insn_ret)
{
    ASSERT(ctx);
    ASSERT(crb_store_branch_ret);
    ASSERT(crb_store_next_ret);
    ASSERT(crb_reg_ret);
    ASSERT(crb_insn_ret);

    const bool is_conditional = ((BO & 0x14) != 0x14);
    const int branch_block = ctx->blocks[ctx->current_block].branch_block;
    const int next_block = ctx->blocks[ctx->current_block].next_block;
    ASSERT(branch_block >= 0);  // Must be valid if we have a label target.

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
        ctx->blocks[branch_block].crb_changed_recursive & crb_dirty;
    const uint32_t crb_dead_next =
        (!is_conditional ? crb_dead_branch :
         next_block < 0 ? 0 :
             ctx->blocks[next_block].crb_changed_recursive & crb_dirty);
    const uint32_t crb_to_kill = crb_dead_branch & crb_dead_next;
    *crb_store_branch_ret = crb_dead_next & ~crb_to_kill;
    *crb_store_next_ret = crb_dead_branch & ~crb_to_kill;
    ASSERT((*crb_store_branch_ret & *crb_store_next_ret) == 0);
    /* These stores are unconditionally dead, so we don't need to save
     * the associated register or value-setting instruction. */
    kill_cr_stores(ctx, BO, BI, crb_to_kill, NULL, NULL);

    /* If crb_dead_branch or crb_dead_next is nonzero at this point, we
     * want to store those bits only on the code path where they are not
     * dead, so we kill the original SET_ALIAS instructions and (if
     * possible) the instructions which set the corresponding RTL register,
     * then re-add them at the branch or fall-through point. */
    const uint32_t crb_to_save = *crb_store_branch_ret | *crb_store_next_ret;
    ASSERT(!(crb_to_kill & crb_to_save));
    kill_cr_stores(ctx, BO, BI, crb_to_save, crb_reg_ret, crb_insn_ret);
}

/*************************************************************************/
/*************************************************************************/
