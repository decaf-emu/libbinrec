/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/bitutils.h"
#include "src/host-x86.h"
#include "src/host-x86/host-x86-internal.h"
#include "src/rtl-internal.h"

/*
 * We use linear scan, as described by Poletto and Sarkar, as the basic
 * algorithm for allocating registers.  We do not maintain an explicit
 * list of live intervals, but we achieve the same effect by iterating
 * over instructions in code-stream order and checking for newly live
 * registers at each instruction.  This is not significantly slower than
 * iterating over a live interval list because SSA implies that most
 * instructions create a new register, so the number of registers -- and
 * thus the number of live ranges -- is of roughly the same order as the
 * number of instructions; by iterating over instructions, we also save
 * the expense of creating a separate list of live ranges.
 *
 * Since live intervals calculated by the RTL core do not take backward
 * branches into account, the register allocator checks each basic block
 * for entering edges from later blocks in the code stream, and if such a
 * block is found, it updates the live intervals of all live registers to
 * extend through the end of that block (the latest block in code stream
 * order if there is more than one).
 *
 * The basic algorithm is modified as follows:
 *
 * - If the FIXED_REGS optimization is enabled, the allocator performs a
 *   preliminary pass over the RTL unit to allocate hardware registers for
 *   operands which must be in specific registers (such as shift counts,
 *   which must be in CL), followed by a second pass to link those
 *   into a list ordered by register birth, before the normal allocation
 *   pass.  The allocator is not strictly linear scan in this sense, but
 *   the extra passes only need to look at a few instructions, so it does
 *   not add a significant amount of time to the overall allocation process.
 *
 * - When spilling registers, the register with the shortest (rather than
 *   longest) usage interval is spilled, in order to avoid spilling the
 *   guest processor state block pointer.
 *   FIXME: instead add a high-priority flag to RTLRegister?
 */

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * get_gpr:  Return the index (X86Register) of the next available GPR, or
 * -1 if no GPRs are available.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     avoid_regs: Bitmask of registers to not use even if free.
 * [Return value]
 *     Next free GPR, or -1 if no GPRs are free.
 */
static inline int get_gpr(HostX86Context *ctx, uint32_t avoid_regs)
{
    const uint32_t regs_free = ctx->regs_free & 0xFFFF & ~avoid_regs;

    /* Give preference to caller-saved registers, so we don't need to
     * unnecessarily save and restore registers ourselves. */
    // FIXME: will need adjustment when we have native calls (probably also want something like NATIVE_CALL_INTERNAL for pre/post insn callbacks that doesn't affect register allocation)
    int host_reg = ctz32(regs_free & ~ctx->callee_saved_regs);
    if (host_reg < 16) {
        return host_reg;
    } else {
        return regs_free ? ctz32(regs_free) : -1;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * get_xmm:  Return the index (X86Register) of the next available XMM
 * register, or -1 if no XMM registers are available.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     avoid_regs: Bitmask of registers to not use even if free.
 * [Return value]
 *     Next free XMM register, or -1 if no XMM registers are free.
 */
static inline int get_xmm(HostX86Context *ctx, uint32_t avoid_regs)
{
    const uint32_t regs_free = ctx->regs_free & 0xFFFF0000 & ~avoid_regs;
    return regs_free ? ctz32(regs_free) : -1;
}

/*-----------------------------------------------------------------------*/

/**
 * allocate_register:  Allocate a host register for the given RTL register.
 * The reg_info->host_allocated flag is not modified.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg_index: RTL register number.
 *     reg: RTLRegister structure for the register.
 *     avoid_regs: Bitmask of registers to not use even if free.
 * [Return value]
 *     Allocated register index.
 */
static X86Register allocate_register(
    HostX86Context *ctx, int reg_index, const RTLRegister *reg,
    uint32_t avoid_regs)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(reg_index > 0);
    ASSERT(reg_index < ctx->unit->next_reg);

    int host_reg;
    if (reg->type == RTLTYPE_INT32 || reg->type == RTLTYPE_ADDRESS) {
        host_reg = get_gpr(ctx, avoid_regs);
    } else {
        host_reg = get_xmm(ctx, avoid_regs);
    }
    if (host_reg >= 0) {
        ASSERT(!ctx->reg_map[host_reg]);
        ctx->reg_map[host_reg] = reg_index;
        ctx->regs_free ^= 1 << host_reg;
        ctx->regs_touched |= 1 << host_reg;
        return host_reg;
    }

    ASSERT(!"FIXME: spilling not yet implemented");
}

/*-----------------------------------------------------------------------*/

/**
 * allocate_regs_for_insn:  Allocate host registers for the given RTL
 * instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of instruction in ctx->unit->insns[].
 * [Return value]
 *     True on success, false on error.
 */
static bool allocate_regs_for_insn(HostX86Context *ctx, int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);

    const RTLUnit * const unit = ctx->unit;
    const RTLInsn * const insn = &unit->insns[insn_index];

    const uint32_t dest = insn->dest;
    ASSERT(dest < unit->next_reg);
    const RTLRegister * const dest_reg = &unit->regs[dest];
    HostX86RegInfo * const dest_info = &ctx->regs[dest];

    const uint32_t src1 = insn->src1;
    ASSERT(src1 < unit->next_reg);
    const RTLRegister * const src1_reg = &unit->regs[src1];
    const HostX86RegInfo * const src1_info = &ctx->regs[src1];

    const uint32_t src2 = insn->src2;
    ASSERT(src2 < unit->next_reg);
    const RTLRegister * const src2_reg = &unit->regs[src2];
    const HostX86RegInfo * const src2_info = &ctx->regs[src2];

    // FIXME: implement CONSTANT_OPERANDS optimization

    if (src1) {
        /* Source registers must have already had a host register allocated
         * (unless they're undefined). */
        if (LIKELY(src1_reg->source != RTLREG_UNDEFINED)) {
            ASSERT(src1_info->host_allocated);
            // FIXME: try to move return values to rAX
            if (src1_reg->death == insn_index) {
                ctx->regs_free |= 1 << src1_info->host_reg;
                ctx->reg_map[src1_info->host_reg] = 0;
            }
        } else {
            /* Extra sanity check to make sure not to leak host registers. */
            ASSERT(!src1_info->host_allocated);
        }
    }

    if (src2) {
        if (LIKELY(src2_reg->source != RTLREG_UNDEFINED)) {
            ASSERT(src2_info->host_allocated);
            if (src2_reg->death == insn_index) {
                ctx->regs_free |= 1 << src2_info->host_reg;
                ctx->reg_map[src2_info->host_reg] = 0;
            }
        } else {
            ASSERT(!src2_info->host_allocated);
        }
    }

    if (insn->opcode == RTLOP_SELECT && insn->cond) {
        ASSERT(insn->cond < unit->next_reg);
        const RTLRegister * const cond_reg = &unit->regs[insn->cond];
        const HostX86RegInfo * const cond_info = &ctx->regs[insn->cond];
        if (LIKELY(cond_reg->source != RTLREG_UNDEFINED)) {
            ASSERT(cond_info->host_allocated);
            if (cond_reg->death == insn_index) {
                ctx->regs_free |= 1 << cond_info->host_reg;
                ctx->reg_map[cond_info->host_reg] = 0;
            }
        } else {
            ASSERT(!cond_info->host_allocated);
        }
    }

    if (dest) {
        /* SSA implies that destination registers should never have been
         * seen before and should therefore never have a hardware register
         * allocated.  However, we may have already allocated a GPR if the
         * register is used in an instruction with fixed operands, so in
         * that case just update the register map. */
        ASSERT(dest_reg->birth == insn_index);

        bool host_allocated = dest_info->host_allocated;
        dest_info->host_allocated = true;  // We'll find one eventually.

        uint32_t avoid_regs = 0;

        if (host_allocated) {
            const X86Register host_reg = dest_info->host_reg;
            ASSERT(!ctx->reg_map[host_reg]);
            ctx->reg_map[host_reg] = dest;
            ASSERT(ctx->regs_free & (1 << host_reg));
            ctx->regs_free ^= 1 << host_reg;
            ctx->regs_touched |= 1 << host_reg;

            /* This must have been the first register on the fixed-regs
             * list.  Advance the list pointer so we don't have to scan
             * over this register again. */
            ASSERT(ctx->fixed_reg_list == dest);
            ctx->fixed_reg_list = dest_info->next_fixed;
        } else {
            /* Make sure not to collide with any registers that have
             * already been allocated. */
            for (uint32_t r = ctx->fixed_reg_list; r;
                 r = ctx->regs[r].next_fixed)
            {
                if (unit->regs[r].birth >= dest_reg->death) {
                    break;
                }
                ASSERT(ctx->regs[r].host_allocated);
                avoid_regs |= 1 << ctx->regs[r].host_reg;
            }
        }

        /* Special case for LOAD_ARG: try to reuse the same register the
         * argument is passed in. */
        // FIXME: only appropriate if no native calls
        if (!host_allocated && insn->opcode == RTLOP_LOAD_ARG) {
            const int target_reg =
                host_x86_int_arg_register(ctx, insn->arg_index);
            if (target_reg < 0) {
                log_error(ctx->handle, "LOAD_ARG %d not supported (argument"
                          " is not in a register)", insn->arg_index);
                return false;
            } else if (!ctx->reg_map[target_reg]
                       && !(avoid_regs & (1 << target_reg))) {
                host_allocated = true;
                dest_info->host_reg = target_reg;
                ctx->reg_map[target_reg] = dest;
                ASSERT(ctx->regs_free & (1 << target_reg));
                ctx->regs_free ^= 1 << target_reg;
                ctx->regs_touched |= 1 << target_reg;
            }
        }

        /*
         * x86 doesn't have separate destination operands for most
         * instructions, so if one of the source operands (if any) dies at
         * this instruction, reuse its host register for the destination
         * to avoid an unnecessary register move.
         *
         * Long multiply and divide are special cases, since the
         * corresponding x86 instructions write to fixed output registers.
         * For those, try to allocate the fixed register if it's available,
         * and make sure to avoid the other (clobbered) output register in
         * any case.
         */
        if (!host_allocated) {
            switch (insn->opcode) {
              case RTLOP_MULHU:
              case RTLOP_MULHS:
              case RTLOP_MODU:
              case RTLOP_MODS:
                if (!ctx->reg_map[X86_DX]) {
                    host_allocated = true;
                    dest_info->host_reg = X86_DX;
                    ctx->reg_map[X86_DX] = dest;
                    ASSERT(ctx->regs_free & (1 << X86_DX));
                    ctx->regs_free ^= 1 << X86_DX;
                    ctx->regs_touched |= 1 << X86_DX;
                } else {
                    avoid_regs |= 1 << X86_AX;
                }
                break;
              case RTLOP_DIVU:
              case RTLOP_DIVS:
                if (!ctx->reg_map[X86_AX]) {
                    host_allocated = true;
                    dest_info->host_reg = X86_AX;
                    ctx->reg_map[X86_AX] = dest;
                    ASSERT(ctx->regs_free & (1 << X86_AX));
                    ctx->regs_free ^= 1 << X86_AX;
                    ctx->regs_touched |= 1 << X86_AX;
                } else {
                    avoid_regs |= 1 << X86_DX;
                }
                break;
              default:
                if (src1 && src1_reg->death == insn_index) {
                    /* The first operand's register can usually be reused
                     * for the destination, except for shifts with src1 in
                     * rCX (since we need CL for the count) and BFINS with
                     * src1==src2 (since we need to write dest before
                     * reading src2). */
                    bool src1_ok;
                    switch (insn->opcode) {
                      case RTLOP_SLL:
                      case RTLOP_SRL:
                      case RTLOP_SRA:
                      case RTLOP_ROR:
                        src1_ok = (src1_info->host_reg != X86_CX);
                        break;
                      case RTLOP_BFINS:
                        src1_ok = (src1_info->host_reg != src2_info->host_reg);
                        break;
                      default:
                        src1_ok = true;
                        break;
                    }
                    if (src1_ok) {
                        host_allocated = true;
                        dest_info->host_reg = src1_info->host_reg;
                        ctx->reg_map[dest_info->host_reg] = dest;
                        ASSERT(ctx->regs_free & (1 << dest_info->host_reg));
                        ctx->regs_free ^= 1 << dest_info->host_reg;
                    }
                }
                if (!host_allocated && src2 && src2_reg->death == insn_index) {
                    /* The second operand's register can only be reused for
                     * commutative operations.  However, RTL SLT/SLE
                     * instructions translate to multiple x86 instructions,
                     * so we can safely reuse src2 for those cases.  DIV/MOD
                     * instructions aren't included here since they don't
                     * reach this test. */
                    static const uint8_t non_commutative[RTLOP__LAST/8 + 1] = {
                        /* Note that opcodes will need to be shifted around
                         * if their numeric values are changed such that
                         * they move to different bytes. */
                        0,
                        1<<(RTLOP_SUB-8),
                        1<<(RTLOP_SLL-16) | 1<<(RTLOP_SRL-16),
                        1<<(RTLOP_SRA-24) | 1<<(RTLOP_ROR-24),
                        1<<(RTLOP_BFINS-32),
                    };
                    ASSERT(insn->opcode >= RTLOP__FIRST
                           && insn->opcode <= RTLOP__LAST);
                    if (non_commutative[insn->opcode/8] & (1 << (insn->opcode%8))) {
                        /* Make sure it's also not chosen by the regular
                         * allocator. */
                        avoid_regs |= 1 << src2_info->host_reg;
                    } else {
                        host_allocated = true;
                        dest_info->host_reg = src2_info->host_reg;
                        ctx->reg_map[dest_info->host_reg] = dest;
                        ASSERT(ctx->regs_free & (1 << dest_info->host_reg));
                        ctx->regs_free ^= 1 << dest_info->host_reg;
                    }
                }
            }  // switch (insn->opcode)
        }   // if (!host_allocated)

        /* If none of the special cases apply, allocate a register normally. */
        if (!host_allocated) {
            /* Be careful not to allocate an unclobberable input register.
             * (Effectively this just means RCX in shift/rotate instructions,
             * since the fixed inputs RAX to MUL/IMUL and rDX:rAX to DIV/IDIV
             * are also outputs.) */
            if (insn->opcode == RTLOP_SLL || insn->opcode == RTLOP_SRL
             || insn->opcode == RTLOP_SRA || insn->opcode == RTLOP_ROR) {
                avoid_regs |= X86_CX;
            }
            dest_info->host_reg =
                allocate_register(ctx, dest, dest_reg, avoid_regs);
        }

        /* Find a temporary register for instructions which need it. */
        bool need_temp;
        uint32_t temp_avoid = avoid_regs;
        switch (insn->opcode) {
          case RTLOP_MULHU:
          case RTLOP_MULHS:
            /* Temporary needed to save RAX if it's live across this insn. */
            need_temp = (ctx->reg_map[X86_AX] != 0);
            temp_avoid |= 1 << ctx->regs[src1].host_reg
                        | 1 << ctx->regs[src2].host_reg;
            break;
          case RTLOP_CLZ:
            /* Temporary needed if using BSR instead of LZCNT to count bits.
             * The temporary can never overlap the input for single-input
             * instructions, so we don't need to explicitly avoid it. */
            need_temp = !(ctx->handle->setup.host_features
                          & BINREC_FEATURE_X86_LZCNT);
            break;
          case RTLOP_BFEXT:
            /* Temporary needed for mask if extracting from the high half
             * of a 64-bit value (but not the very top, since that's
             * implemented with a simple shift). */
            need_temp = (dest_reg->type == RTLTYPE_ADDRESS
                         && insn->bitfield.start + insn->bitfield.count < 64
                         && insn->bitfield.count > 32);
            break;
          case RTLOP_BFINS:
            /* Temporary needed if inserting into a 64-bit src1 whose
             * register is reused as the destination (so we have somewhere
             * to put the mask), or if src2 remains live past this
             * instruction (so we can't mask and shift it in place). */
            need_temp = ((dest_reg->type == RTLTYPE_ADDRESS
                          && dest_info->host_reg == src1_info->host_reg)
                         || src2_reg->death > insn_index);
            temp_avoid |= 1 << ctx->regs[src1].host_reg
                        | 1 << ctx->regs[src2].host_reg;
            break;
          default:
            need_temp = false;
            break;
        }
        if (need_temp) {
            int temp_reg = get_gpr(ctx, temp_avoid);
            if (temp_reg < 0) {
                ASSERT(!"FIXME: spilling not yet implemented");
            }
            dest_info->host_temp = (uint8_t)temp_reg;
            dest_info->temp_allocated = true;
            ctx->regs_touched |= 1 << temp_reg;
        }

        /* If the register isn't referenced again, immediately free it.
         * Normally such instructions will be optimized out, but if
         * optimization is disabled, for example, we shouldn't leave the
         * register allocated forever. */
        if (UNLIKELY(dest_reg->death == insn_index)) {
            ctx->regs_free |= 1 << dest_info->host_reg;
            ctx->reg_map[dest_info->host_reg] = 0;
        }
    }  // if (dest)

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * allocate_regs_for_block:  Allocate host registers for RTL registers in
 * the given basic block.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 * [Return value]
 *     True on success, false on error.
 */
static bool allocate_regs_for_block(HostX86Context *ctx, int block_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(block_index >= 0);
    ASSERT(block_index < ctx->unit->num_blocks);

    const RTLBlock * const block = &ctx->unit->blocks[block_index];

    STATIC_ASSERT(sizeof(ctx->blocks[block_index].initial_reg_map)
                  == sizeof(ctx->reg_map), "Mismatched reg_map sizes");
    memcpy(ctx->blocks[block_index].initial_reg_map, ctx->reg_map,
           sizeof(ctx->reg_map));

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        if (!allocate_regs_for_insn(ctx, insn_index)) {
            return false;
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * allocate_fixed_regs_for_block:  Allocate host registers for RTL
 * registers with allocation constraints in the given basic block.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 */
static void allocate_fixed_regs_for_block(HostX86Context *ctx, int block_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(block_index >= 0);
    ASSERT(block_index < ctx->unit->num_blocks);

    const RTLUnit * const unit = ctx->unit;
    const RTLBlock * const block = &unit->blocks[block_index];

    STATIC_ASSERT(sizeof(ctx->blocks[block_index].initial_reg_map)
                  == sizeof(ctx->reg_map), "Mismatched reg_map sizes");
    memcpy(ctx->blocks[block_index].initial_reg_map, ctx->reg_map,
           sizeof(ctx->reg_map));

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        const RTLInsn * const insn = &unit->insns[insn_index];

        switch (insn->opcode) {
          case RTLOP_MULHU:
          case RTLOP_MULHS: {
            /* MUL and IMUL read rAX and write rDX:rAX, so ideally we want
             * one input operand in rAX, the other in rDX if it dies at this
             * instruction, and the result in rDX since these instructions
             * return the high word of the result. */

            const RTLRegister *dest_reg = &unit->regs[insn->dest];
            HostX86RegInfo *dest_info = &ctx->regs[insn->dest];
            const RTLRegister *src1_reg = &unit->regs[insn->src1];
            HostX86RegInfo *src1_info = &ctx->regs[insn->src1];
            const RTLRegister *src2_reg = &unit->regs[insn->src2];
            HostX86RegInfo *src2_info = &ctx->regs[insn->src2];

            if (ctx->last_dx_death <= insn_index) {
                ASSERT(dest_reg->birth == insn_index);
                dest_info->host_allocated = true;
                dest_info->host_reg = X86_DX;
                /* If both src1 and src2 are candidates for getting DX,
                 * choose the one which was born earlier, since the other
                 * will have a better chance of getting AX below. */
                const bool src1_ok = (!src1_info->host_allocated
                                      && src1_reg->death == insn_index
                                      && src1_reg->birth > ctx->last_dx_death);
                const bool src2_ok = (!src2_info->host_allocated
                                      && src2_reg->death == insn_index
                                      && src2_reg->birth > ctx->last_dx_death);
                if (src1_ok && (!src2_ok || src1_reg->birth < src2_reg->birth)) {
                    src1_info->host_allocated = true;
                    src1_info->host_reg = X86_DX;
                } else if (src2_ok) {
                    src2_info->host_allocated = true;
                    src2_info->host_reg = X86_DX;
                }
                ctx->last_dx_death = dest_reg->death;
            }

            if (!src1_info->host_allocated && src1_reg->birth > ctx->last_ax_death) {
                src1_info->host_allocated = true;
                src1_info->host_reg = X86_AX;
                ctx->last_ax_death = src1_reg->death;
            } else if (!src2_info->host_allocated && src2_reg->birth > ctx->last_ax_death) {
                src2_info->host_allocated = true;
                src2_info->host_reg = X86_AX;
                ctx->last_ax_death = src2_reg->death;
            }

            break;
          }  // case RTLOP_MULH[US]

          case RTLOP_SLL:
          case RTLOP_SRL:
          case RTLOP_SRA:
          case RTLOP_ROR: {
            const RTLRegister *src2_reg = &unit->regs[insn->src2];
            HostX86RegInfo *src2_info = &ctx->regs[insn->src2];
            if (!src2_info->host_allocated && src2_reg->birth > ctx->last_cx_death) {
                src2_info->host_allocated = true;
                src2_info->host_reg = X86_CX;
                ctx->last_cx_death = src2_reg->death;
            }
            break;
          }  // case RTLOP_{SLL,SRL,SRA,ROR}

          default:
            break;  // Nothing to do in this pass.
        }
    }
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

bool host_x86_allocate_registers(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);

    RTLUnit * const unit = ctx->unit;

    if (ctx->handle->setup.host == BINREC_ARCH_X86_64_SYSV) {
        ctx->callee_saved_regs =
            1<<X86_BX | 1<<X86_BP | 1<<X86_R12 | 1<<X86_R13
            | 1<<X86_R14 | 1<<X86_R15;
    } else {
        ASSERT(ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS
            || ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);
        ctx->callee_saved_regs =
            1<<X86_BX | 1<<X86_BP | 1<<X86_SI | 1<<X86_DI
            | 1<<X86_R12 | 1<<X86_R13 | 1<<X86_R14 | 1<<X86_R15
            | 1<<X86_XMM6 | 1<<X86_XMM7 | 1<<X86_XMM8 | 1<<X86_XMM9
            | 1<<X86_XMM10 | 1<<X86_XMM11 | 1<<X86_XMM12 | 1<<X86_XMM13
            | 1<<X86_XMM14 | 1<<X86_XMM15;
    }

    memset(ctx->reg_map, 0, sizeof(ctx->reg_map));
    ctx->regs_free = ~UINT32_C(0) ^ 1<<X86_SP;  // Don't try to allocate SP!
    ctx->regs_touched = 0;

    if (ctx->handle->host_opt & BINREC_OPT_H_X86_FIXED_REGS) {
        ctx->last_ax_death = -1;
        ctx->last_cx_death = -1;
        ctx->last_dx_death = -1;

        for (int block_index = 0; block_index >= 0;
             block_index = unit->blocks[block_index].next_block)
        {
            allocate_fixed_regs_for_block(ctx, block_index);
        }

        int last_fixed_reg = 0;
        for (int block_index = 0; block_index >= 0;
             block_index = unit->blocks[block_index].next_block)
        {
            const RTLBlock * const block = &unit->blocks[block_index];
            for (int insn_index = block->first_insn;
                 insn_index <= block->last_insn; insn_index++)
            {
                const RTLInsn * const insn = &unit->insns[insn_index];
                if (insn->dest && ctx->regs[insn->dest].host_allocated) {
                    if (last_fixed_reg) {
                        ctx->regs[last_fixed_reg].next_fixed = insn->dest;
                    } else {
                        ctx->fixed_reg_list = insn->dest;
                    }
                    last_fixed_reg = insn->dest;
                }
            }
        }
    }

    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        if (!allocate_regs_for_block(ctx, block_index)) {
            return false;
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

int host_x86_int_arg_register(HostX86Context *ctx, int index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(index >= 0);

    if (ctx->handle->setup.host == BINREC_ARCH_X86_64_SYSV) {
        static const uint8_t regs[6] =
            {X86_DI, X86_SI, X86_DX, X86_CX, X86_R8, X86_R9};
        if (index < lenof(regs)) {
            return regs[index];
        } else {
            return -1;
        }
    } else {
        ASSERT(ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS
            || ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);
        static const uint8_t regs[4] = {X86_CX, X86_DX, X86_R8, X86_R9};
        if (index < lenof(regs)) {
            return regs[index];
        } else {
            return -1;
        }
    }
}

/*************************************************************************/
/*************************************************************************/
