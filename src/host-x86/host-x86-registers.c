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
 * FIXME: figure out spilling rules ("latest death" would spill the PSB pointer, which would not be great, but we do want to spill longer-lived regs in general, esp. because of alias stuff)
 *
 * Since live intervals calculated by the RTL core do not take backward
 * branches into account, the register allocator checks each basic block
 * for entering edges from later blocks in the code stream, and if such a
 * block is found, it updates the live intervals of all live registers to
 * extend through the end of that block (the latest block in code stream
 * order if there is more than one).
 *
 * Before the register allocation pass itself, we scan through the code
 * stream once to record which RTL registers are stored in which aliases
 * at the end of each block; this information is used to try and allocate
 * the same hardware register to RTL registers linked to the same alias.
 * If the FIXED_REGS optimization is enabled, then during the alias scan,
 * we also allocate hardware registers for operands which must be in
 * specific registers (such as shift counts, which must be in CL); we do
 * this as part of the same pass to avoid the cost of looping over the
 * entire unit an additional time.
 *
 * Resolution of alias references occurs in several stages, depending on
 * the nature of the reference:
 *
 * - Multiple loads (GET_ALIAS) and stores (SET_ALIAS) to the same alias
 *   within the same basic block are resolved during the first pass by
 *   converting load-after-load and load-after-store to MOVE.
 *   Store-after-store is handled by ignoring the earlier (dead) store at
 *   code generation time.
 *
 * - After the first pass, the source registers for all remaining alias
 *   stores have their live ranges extended to the end of the block
 *   containing the store if a successor block loads the same alias (see
 *   update_alias_live_ranges()).
 *
 * - For loads not rewritten in the first pass, if at least one predecessor
 *   block (that is, a block with a flow graph edge entering the current
 *   block) stores to the alias, the register allocator will attempt to
 *   allocate a host register which has not yet been used in the current
 *   block, giving preference to the register used to store to the alias
 *   in the predecessor block.  (If this applies to multiple predecessor
 *   blocks, first preference is given to the immediately previous block
 *   in code stream order if that block is a predecessor block.)  If this
 *   is successful, the corresponding RTL register is marked as valid for
 *   alias merging during code generation.  Otherwise, a host register is
 *   allocated at the point of the load as usual.
 *
 * - At code generation time, stores and loads of aliases marked as valid
 *   for merging are ignored.  Instead, at the exit point of the block
 *   containing the store (immediately before the branch instruction, if
 *   one is to be generated), the target host register is loaded with the
 *   proper value, so it will be valid on entry to the block containing
 *   the load.  ("Ignored" loads may still generate a MOV instruction if
 *   the register used to carry the alias value between blocks can't be
 *   reused in subsequent instructions due to operand constraints.)
 *
 * - All other loads and stores are replaced by memory load and store
 *   instructions referencing the alias's storage.  Merged stores are also
 *   stored to memory if the successor block does not store the alias
 *   itself.
 *   FIXME: note that this has implications for spilling due to live range extension -- if the register is live through the end of the block but ends up getting stored because merged alloc failed, and it also ends up getting spilled due to later code (after the SET_ALIAS) in the block, we can ignore the spill; this does however mean that we need to match spill location to alias storage, so if the register is used for something else later in the block, it's properly reloaded
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
    // FIXME: will need adjustment when we have native calls (probably also want something like NATIVE_CALL_INTERNAL for pre/post insn callbacks that shouldn't affect register allocation)
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
 * assign_register:  Assign the given host register to the given RTL
 * register.  Updates ctx->reg_map, ctx->regs_free, and
 * ctx->block_regs_touched, but does not update HostX86RegInfo.host_reg.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg_index: RTL register number.
 *     host_reg: Host register to assign.
 * [Return value]
 *     Allocated register index.
 */
static void assign_register(HostX86Context *ctx, int reg_index,
                            X86Register host_reg)
{
    ASSERT(ctx);

    ASSERT(!ctx->reg_map[host_reg]);
    ctx->reg_map[host_reg] = reg_index;
    ASSERT(ctx->regs_free & (1 << host_reg));
    ctx->regs_free ^= 1 << host_reg;
    ctx->block_regs_touched |= 1 << host_reg;
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
        assign_register(ctx, reg_index, host_reg);
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
 *     block_index: Index of basic block containing instruction.
 * [Return value]
 *     True on success, false on error.
 */
static bool allocate_regs_for_insn(HostX86Context *ctx, int32_t insn_index,
                                   int block_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);
    ASSERT(block_index >= 0);
    ASSERT((unsigned int)block_index < ctx->unit->num_blocks);

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

        uint32_t avoid_regs = dest_info->avoid_regs;

        if (host_allocated) {
            const X86Register host_reg = dest_info->host_reg;
            assign_register(ctx, dest, host_reg);
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

        /* If loading from an alias which was written by a predecessor
         * block, try to keep the value live in a host register.  We check
         * for this even if a host register was allocated for dest during
         * optimization, since we may still be able to reserve that
         * register as a block input or find another register to serve as
         * an intermediary to avoid the memory store/load. */
        if (insn->opcode == RTLOP_GET_ALIAS) {
            const int alias = insn->alias;
            ASSERT(ctx->blocks[block_index].alias_load[alias] == dest);
            bool have_preceding_store = false;

            /* First priority: register used by SET_ALIAS in previous block
             * (if any, and if it has an edge to this block). */
            if (block_index > 0
             && (unit->blocks[block_index-1].exits[0] == block_index
              || unit->blocks[block_index-1].exits[1] == block_index)) {
                const int store_reg =
                    ctx->blocks[block_index-1].alias_store[alias];
                if (store_reg) {
                    have_preceding_store = true;
                    const X86Register host_reg = ctx->regs[store_reg].host_reg;
                    if (!(ctx->block_regs_touched & (1 << host_reg))) {
                        dest_info->merge_alias = true;
                        dest_info->host_merge = dest_info->host_reg;
                    }
                }
            }

            /* Second priority: register used by SET_ALIAS in any
             * predecessor block. */
            if (!dest_info->merge_alias) {
                RTLBlock *block = &unit->blocks[block_index];
                for (int i = 0;
                     i < lenof(block->entries) && block->entries[i] >= 0; i++)
                {
                    if (block->entries[i] == block_index - 1) {
                        continue;  // Already checked this block above.
                    }
                    const HostX86BlockInfo *predecessor =
                        &ctx->blocks[block->entries[i]];
                    const int store_reg = predecessor->alias_store[alias];
                    if (store_reg) {
                        have_preceding_store = true;
                        const X86Register host_reg =
                            ctx->regs[store_reg].host_reg;
                        if (!(ctx->block_regs_touched & (1 << host_reg))) {
                            dest_info->merge_alias = true;
                            dest_info->host_merge = dest_info->host_reg;
                        }
                    }
                }
            }

            /* Zeroth priority: If we already have a register allocated
             * (from optimization) and that register is free back to the
             * beginning of the block, use it.  We delay this check until
             * now since we need to know whether to allocate a register in
             * the first place (i.e. have_preceding_store). */
            if (have_preceding_store && host_allocated
             && !(ctx->block_regs_touched & (1 << dest_info->host_reg))) {
                /* The register is available!  Claim it for our own. */
                dest_info->merge_alias = true;
                dest_info->host_merge = dest_info->host_reg;
            }

            /* Lowest priority: any register not yet used in this block.
             * But if there are no predecessor blocks which set this alias,
             * there's nothing to merge, so don't even try to allocate a
             * register. */
            if (!dest_info->merge_alias && have_preceding_store) {
                const RTLDataType type = dest_reg->type;
                const uint32_t saved_free = ctx->regs_free;
                ctx->regs_free = ~ctx->block_regs_touched;
                int host_reg;
                if (type == RTLTYPE_INT32 || type == RTLTYPE_ADDRESS) {
                    host_reg = get_gpr(ctx, avoid_regs);
                } else {
                    host_reg = get_xmm(ctx, avoid_regs);
                }
                if (host_reg < 0 && avoid_regs != 0) {
                    /* Try again without excluding any registers.  We won't
                     * be able to keep the value in this register, but we
                     * can at least avoid a round trip to memory. */
                    if (type == RTLTYPE_INT32 || type == RTLTYPE_ADDRESS) {
                        host_reg = get_gpr(ctx, 0);
                    } else {
                        host_reg = get_xmm(ctx, 0);
                    }
                }
                ctx->regs_free = saved_free;
                if (host_reg >= 0) {
                    dest_info->merge_alias = true;
                    dest_info->host_merge = host_reg;
                }
            }

            /* If we found a usable register, assign it to dest.  But if
             * dest already had a register (from optimization) or if the
             * chosen register is masked out by avoid_regs, just mark the
             * register as touched so subsequent alias loads don't also try
             * to use it; the code generator will construct a MOV to get
             * the value to its proper location. */
            if (dest_info->merge_alias) {
                const X86Register host_merge = dest_info->host_merge;
                if (host_allocated || (avoid_regs & (1 << host_merge))) {
                    ctx->block_regs_touched |= 1 << host_merge;
                } else {
                    host_allocated = true;
                    dest_info->host_reg = host_merge;
                    assign_register(ctx, dest, host_merge);
                }
            }
        }  // if (insn->opcode == RTLOP_GET_ALIAS)

        /* If loading a function argument, try to reuse the same register
         * the argument is passed in. */
        // FIXME: only appropriate if no native calls
        // FIXME: need more robustness wrt overwriting input regs
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
                assign_register(ctx, dest, target_reg);
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
         * any case, but don't try to reuse source registers since that
         * gives no benefit.  In the case of division (DIV[US]/MOD[US]),
         * we actually need to avoid reusing src2 because of all the fixed
         * register clobbering.
         */
        if (!host_allocated) {
            switch (insn->opcode) {
              case RTLOP_MULHU:
              case RTLOP_MULHS:
                if (!ctx->reg_map[X86_DX]) {
                    host_allocated = true;
                    dest_info->host_reg = X86_DX;
                    assign_register(ctx, dest, X86_DX);
                } else {
                    avoid_regs |= 1 << X86_AX;
                }
                break;
              case RTLOP_DIVU:
              case RTLOP_DIVS:
                if (!ctx->reg_map[X86_AX] && src2_info->host_reg != X86_AX) {
                    host_allocated = true;
                    dest_info->host_reg = X86_AX;
                    assign_register(ctx, dest, X86_AX);
                } else {
                    avoid_regs |= 1 << X86_AX
                                | 1 << X86_DX
                                | 1 << src2_info->host_reg;
                }
                break;
              case RTLOP_MODU:
              case RTLOP_MODS:
                if (!ctx->reg_map[X86_DX] && src2_info->host_reg != X86_DX) {
                    host_allocated = true;
                    dest_info->host_reg = X86_DX;
                    assign_register(ctx, dest, X86_DX);
                } else {
                    avoid_regs |= 1 << X86_AX
                                | 1 << X86_DX
                                | 1 << src2_info->host_reg;
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
                        /* src1==src2 should normally never happen (unless
                         * the input is doing something bizarre), but if it
                         * does it'll confuse the translator, so avoid that
                         * case as well. */
                        src1_ok = (src1_info->host_reg != X86_CX
                                   && src1 != src2);
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
                break;
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
                avoid_regs |= 1 << X86_CX;
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
          case RTLOP_DIVU:
          case RTLOP_DIVS:
            /* Temporary needed to save RDX if it's live across this insn
             * or if it's allocated to src2 (even if src2 dies here). */
            need_temp = (ctx->reg_map[X86_DX] != 0
                         || ctx->regs[src2].host_reg == X86_DX);
            temp_avoid |= 1 << ctx->regs[src1].host_reg
                        | 1 << ctx->regs[src2].host_reg;
            break;
          case RTLOP_MODU:
          case RTLOP_MODS:
            /* Temporary needed to save RAX if it's live across this insn
             * or if it's allocated to src2 (even if src2 dies here). */
            need_temp = (ctx->reg_map[X86_AX] != 0
                         || ctx->regs[src2].host_reg == X86_AX);
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
            ctx->block_regs_touched |= 1 << temp_reg;
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

    const RTLUnit * const unit = ctx->unit;
    const RTLBlock * const block = &unit->blocks[block_index];

    STATIC_ASSERT(sizeof(ctx->blocks[block_index].initial_reg_map)
                  == sizeof(ctx->reg_map), "Mismatched reg_map sizes");
    memcpy(ctx->blocks[block_index].initial_reg_map, ctx->reg_map,
           sizeof(ctx->reg_map));
    ctx->block_regs_touched = 0;

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        if (!allocate_regs_for_insn(ctx, insn_index, block_index)) {
            return false;
        }
    }

    /* Registers used in SET_ALIAS instructions may have had their live
     * ranges extended past the last instruction that referenced them.
     * Make sure to properly free their host registers. */
    const int32_t last_insn = block->last_insn;
    const HostX86BlockInfo *block_info = &ctx->blocks[block_index];
    for (int alias = 1; alias < unit->next_alias; alias++) {
        const int reg = block_info->alias_store[alias];
        if (reg && unit->regs[reg].death == last_insn) {
            const HostX86RegInfo *reg_info = &ctx->regs[reg];
            ASSERT(reg_info->host_allocated);
            const X86Register host_reg = reg_info->host_reg;
            if (ctx->reg_map[host_reg] == reg) {
                ctx->reg_map[host_reg] = 0;
                ctx->regs_free |= 1 << host_reg;
            }
        }
    }

    ctx->regs_touched |= ctx->block_regs_touched;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * first_pass_for_block:  Collect data for aliases referenced by the given
 * basic block, and allocate host registers for RTL registers with
 * allocation constraints if the corresponding optimization is enabled.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 */
static void first_pass_for_block(HostX86Context *ctx, int block_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(block_index >= 0);
    ASSERT(block_index < ctx->unit->num_blocks);

    const RTLUnit * const unit = ctx->unit;
    const RTLBlock * const block = &unit->blocks[block_index];
    HostX86BlockInfo * const block_info = &ctx->blocks[block_index];
    const bool do_fixed_regs =
        (ctx->handle->host_opt & BINREC_OPT_H_X86_FIXED_REGS) != 0;

    const int num_aliases = unit->next_alias;
    block_info->alias_load =
        (uint16_t *)&ctx->alias_buffer[block_index * (4 * num_aliases)];
    block_info->alias_store =
        (uint16_t *)&ctx->alias_buffer[block_index * (4 * num_aliases)
                                       + (2 * num_aliases)];

    /* If this block has exactly one entering edge and that edge comes from
     * a block we've already seen, carry alias-store data over from that
     * block to try and keep the alias values in host registers. */
    const bool forward_alias_store = (
        block->entries[0] >= 0 && block->entries[0] < block_index
        && block->entries[1] < 0);
    const uint16_t *predecessor_store = NULL;
    if (forward_alias_store) {
        predecessor_store = ctx->blocks[block->entries[0]].alias_store;
    }

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        RTLInsn * const insn = &unit->insns[insn_index];

        switch (insn->opcode) {
          case RTLOP_GET_ALIAS:
            if (UNLIKELY(block_info->alias_load[insn->alias])) {
                /* We already loaded the alias once!  Probably a lazy guest
                 * translator.  Just reuse the register. */
                insn->src1 = block_info->alias_load[insn->alias];
                insn->opcode = RTLOP_MOVE;
                if (unit->regs[insn->src1].death < insn_index) {
                    unit->regs[insn->src1].death = insn_index;
                }
            } else if (UNLIKELY(block_info->alias_store[insn->alias])) {
                /* We already stored the alias in this block!  Reuse the
                 * register. */
                insn->src1 = block_info->alias_store[insn->alias];
                insn->opcode = RTLOP_MOVE;
                if (unit->regs[insn->src1].death < insn_index) {
                    unit->regs[insn->src1].death = insn_index;
                }
            } else {
                block_info->alias_load[insn->alias] = insn->dest;
                /* We don't convert forwarded stores to MOVE in order to
                 * give the register allocator leeway to use a different
                 * register between the beginning of the block and this
                 * instruction. */
            }
            break;

          case RTLOP_SET_ALIAS:
            block_info->alias_store[insn->alias] = insn->src1;
            break;

          case RTLOP_MULHU:
          case RTLOP_MULHS: {
            if (!do_fixed_regs) {
                break;
            }

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
                /* Currently, all cases where we avoid rAX/rDX are for
                 * registers born before the most recent death of that
                 * host register, so we don't need to check avoid_regs
                 * here (and we omit the check since there's no way to
                 * test it).  We do keep assertions around to catch
                 * problems in case future additions to the logic render
                 * this assumption invalid. */
                ASSERT(!(dest_info->avoid_regs & (1 << X86_DX)));
                dest_info->host_allocated = true;
                dest_info->host_reg = X86_DX;
                /* If both src1 and src2 are candidates for getting DX,
                 * choose the one which was born earlier, since the other
                 * will have a better chance of getting AX below. */
                const bool src1_ok = (!src1_info->host_allocated
                                      && src1_reg->death == insn_index
                                      && src1_reg->birth >= ctx->last_dx_death);
                const bool src2_ok = (!src2_info->host_allocated
                                      && src2_reg->death == insn_index
                                      && src2_reg->birth >= ctx->last_dx_death);
                if (src1_ok && (!src2_ok || src1_reg->birth < src2_reg->birth)) {
                    ASSERT(!(src1_info->avoid_regs & (1 << X86_DX)));
                    src1_info->host_allocated = true;
                    src1_info->host_reg = X86_DX;
                } else if (src2_ok) {
                    ASSERT(!(src2_info->avoid_regs & (1 << X86_DX)));
                    src2_info->host_allocated = true;
                    src2_info->host_reg = X86_DX;
                }
                ctx->last_dx_death = dest_reg->death;
            }

            if (!src1_info->host_allocated && src1_reg->birth >= ctx->last_ax_death) {
                ASSERT(!(src1_info->avoid_regs & (1 << X86_AX)));
                src1_info->host_allocated = true;
                src1_info->host_reg = X86_AX;
                ctx->last_ax_death = src1_reg->death;
            } else if (!src2_info->host_allocated && src2_reg->birth >= ctx->last_ax_death) {
                ASSERT(!(src2_info->avoid_regs & (1 << X86_AX)));
                src2_info->host_allocated = true;
                src2_info->host_reg = X86_AX;
                ctx->last_ax_death = src2_reg->death;
            }

            break;
          }  // case RTLOP_MULH[US]

          case RTLOP_DIVU:
          case RTLOP_DIVS: {
            if (!do_fixed_regs) {
                break;
            }
            /* For div/mod, we only care about putting the result in rAX or
             * rDX (as appropriate) and the dividend in rAX.  Putting the
             * divisor in either rAX or rDX will just force us to move it
             * out of the way later. */
            const RTLRegister *dest_reg = &unit->regs[insn->dest];
            HostX86RegInfo *dest_info = &ctx->regs[insn->dest];
            if (ctx->last_ax_death <= insn_index) {
                ASSERT(dest_reg->birth == insn_index);
                /* We require that dest and src2 not share the same
                 * register, so if src2 is already in rAX, we can't assign
                 * it to dest here. */
                HostX86RegInfo *src2_info = &ctx->regs[insn->src2];
                if (!(src2_info->host_allocated
                      && src2_info->host_reg == X86_AX)) {
                    ASSERT(!(dest_info->avoid_regs & (1 << X86_AX)));
                    dest_info->host_allocated = true;
                    dest_info->host_reg = X86_AX;
                    /* Make sure src2 doesn't get rAX in the main
                     * allocation pass. */
                    src2_info->avoid_regs |= 1 << X86_AX;
                    const RTLRegister *src1_reg = &unit->regs[insn->src1];
                    HostX86RegInfo *src1_info = &ctx->regs[insn->src1];
                    if (!src1_info->host_allocated
                     && src1_reg->death == insn_index
                     && src1_reg->birth >= ctx->last_ax_death) {
                        ASSERT(!(src1_info->avoid_regs & (1 << X86_AX)));
                        src1_info->host_allocated = true;
                        src1_info->host_reg = X86_AX;
                    }
                    ctx->last_ax_death = dest_reg->death;
                }
            }
            break;
          }  // case RTLOP_DIV[US]

          case RTLOP_MODU:
          case RTLOP_MODS: {
            if (!do_fixed_regs) {
                break;
            }
            const RTLRegister *dest_reg = &unit->regs[insn->dest];
            HostX86RegInfo *dest_info = &ctx->regs[insn->dest];
            if (ctx->last_dx_death <= insn_index) {
                ASSERT(dest_reg->birth == insn_index);
                HostX86RegInfo *src2_info = &ctx->regs[insn->src2];
                if (!(src2_info->host_allocated
                      && src2_info->host_reg == X86_DX)) {
                    ASSERT(!(dest_info->avoid_regs & (1 << X86_DX)));
                    dest_info->host_allocated = true;
                    dest_info->host_reg = X86_DX;
                    src2_info->avoid_regs |= 1 << X86_DX;
                    ctx->last_dx_death = dest_reg->death;
                }
            }
            const RTLRegister *src1_reg = &unit->regs[insn->src1];
            HostX86RegInfo *src1_info = &ctx->regs[insn->src1];
            if (!src1_info->host_allocated && src1_reg->birth > ctx->last_ax_death) {
                ASSERT(!(src1_info->avoid_regs & (1 << X86_AX)));
                src1_info->host_allocated = true;
                src1_info->host_reg = X86_AX;
                ctx->last_ax_death = src1_reg->death;
            }
            break;
          }  // case RTLOP_MOD[US]

          case RTLOP_SLL:
          case RTLOP_SRL:
          case RTLOP_SRA:
          case RTLOP_ROR: {
            if (!do_fixed_regs) {
                break;
            }
            const RTLRegister *src2_reg = &unit->regs[insn->src2];
            HostX86RegInfo *src2_info = &ctx->regs[insn->src2];
            if (!src2_info->host_allocated
             && !(src2_info->avoid_regs & (1 << X86_CX))
             && src2_reg->birth >= ctx->last_cx_death) {
                src2_info->host_allocated = true;
                src2_info->host_reg = X86_CX;
                ctx->last_cx_death = src2_reg->death;
            }
            /* Make sure rCX isn't allocated to this register later, since
             * the translator doesn't support rCX as a shift destination. */
            ctx->regs[insn->dest].avoid_regs |= 1 << X86_CX;
            break;
          }  // case RTLOP_{SLL,SRL,SRA,ROR}

          default:
            break;  // Nothing to do in this pass.
        }
    }

    /* Forward alias store data if appropriate.  Aliases which were loaded
     * but not stored in this block are _not_ forwarded, so the code
     * generator knows that it needs to generate a memory store at the
     * earlier SET_ALIAS instruction. */
    if (forward_alias_store) {
        for (int i = 1; i < unit->next_alias; i++) {
            if (!block_info->alias_load[i] && !block_info->alias_store[i]) {
                block_info->alias_store[i] = predecessor_store[i];
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * update_alias_live_ranges:  Extend live ranges of registers associated
 * with aliases read by the given basic block so they are live when
 * entering this block.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 */
static void update_alias_live_ranges(HostX86Context *ctx, int block_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(block_index >= 0);
    ASSERT(block_index < ctx->unit->num_blocks);

    const RTLUnit * const unit = ctx->unit;
    const int num_aliases = unit->next_alias;
    const RTLBlock * const block = &unit->blocks[block_index];
    HostX86BlockInfo * const block_info = &ctx->blocks[block_index];

    if (block->entries[0] < 0) {
        return;  // Nothing to do for the initial block.
    }

    for (int alias = 1; alias < num_aliases; alias++) {
        if (!block_info->alias_load[alias]) {
            continue;
        }

        for (int i = 0;
             i < lenof(block->entries) && block->entries[i] >= 0; i++)
        {
            const int predecessor = block->entries[i];
            const int reg = ctx->blocks[predecessor].alias_store[alias];
            if (reg) {
                const int32_t last_insn = unit->blocks[predecessor].last_insn;
                if (unit->regs[reg].death < last_insn) {
                    unit->regs[reg].death = last_insn;
                }
            }
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

    /* Reserve stack space for any aliases without bound storage. */
    for (int i = 1; i < unit->next_alias; i++) {
        RTLAlias *alias = &unit->aliases[i];
        if (!alias->base) {
            int size = 0;
            switch (alias->type) {
                case RTLTYPE_INT32:     size =  4; break;
                case RTLTYPE_ADDRESS:   size =  8; break;
                case RTLTYPE_FLOAT:     size =  4; break;
                case RTLTYPE_DOUBLE:    size =  8; break;
                case RTLTYPE_V2_DOUBLE: size = 16; break;
            }
            ASSERT(size > 0);
            ctx->frame_size = align_up(ctx->frame_size, size);
            alias->offset = ctx->frame_size;
            ctx->frame_size += size;
        }
    }

    /* First pass: record alias info, and allocate fixed regs if enabled. */
    if (ctx->handle->host_opt & BINREC_OPT_H_X86_FIXED_REGS) {
        ctx->last_ax_death = -1;
        ctx->last_cx_death = -1;
        ctx->last_dx_death = -1;
    }
    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        first_pass_for_block(ctx, block_index);
    }

    /* Generate sorted list of registers allocated by fixed-regs allocation.
     * We do this in a separate pass since it can be done in linear time,
     * as opposed to ordered insertion during allocation (which won't
     * always be appending to the end of the list because we allocate some
     * registers at point of use rather than definition). */
    if (ctx->handle->host_opt & BINREC_OPT_H_X86_FIXED_REGS) {
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

    /* Extend live ranges for registers linked through aliases. */
    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        update_alias_live_ranges(ctx, block_index);
    }

    /* Second pass: allocate hardware registers to all (remaining) RTL
     * registers. */
    memset(ctx->reg_map, 0, sizeof(ctx->reg_map));
    ctx->regs_free = ~UINT32_C(0) ^ 1<<X86_SP;  // Don't try to allocate SP!
    ctx->regs_touched = 0;
    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        if (!allocate_regs_for_block(ctx, block_index)) {
            return false;
        }
    }

    /* x86-64 requires a 16-byte-aligned stack, and the code generator
     * expects us to give it a properly aligned frame size in order to
     * keep the stack aligned after saving registers. */
    ctx->frame_size = align_up(ctx->frame_size, 16);

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
