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
 * number of instructions, and by iterating over instructions we save the
 * expense of creating a separate list of live ranges.
 *
 * Since live intervals calculated by the RTL core do not take backward
 * branches into account, the register allocator checks each basic block
 * for entering edges from later blocks in the code stream, and if such a
 * block is found, it updates the live intervals of all live registers to
 * extend through the end of that block (the latest block in code stream
 * order if there is more than one).
 *
 * The basic algorithm is tweaked as follows:
 *
 * - When spilling registers, the register with the shortest (rather than
 *   longest) usage interval is spilled, in order to avoid spilling the
 *   guest processor state block pointer.
 */

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * allocate_register:  Allocate a host register for the given RTL register.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register number.
 */
static void allocate_register(HostX86Context *ctx, int reg_index)
{
    ASSERT(ctx);
    ASSERT(ctx->unit);
    ASSERT(reg_index > 0);
    ASSERT(reg_index < ctx->unit->next_reg);

    const RTLRegister *reg = &ctx->unit->regs[reg_index];
    HostX86RegInfo *reg_info = &ctx->regs[reg_index];
    ASSERT(!reg_info->host_allocated);

    uint32_t regs_free;
    int reg_base;
    if (reg->type == RTLTYPE_INT32 || reg->type == RTLTYPE_ADDRESS) {
        regs_free = ctx->regs_free & 0xFFFF;
        reg_base = 0;
    } else {
        regs_free = ctx->regs_free & 0xFFFF0000;
        reg_base = 16;
    }

    /* Give preference to caller-saved registers, so we don't need to
     * unnecessarily save and restore registers ourselves. */
    // FIXME: will need adjustment when we have native calls (probably also want something like NATIVE_CALL_INTERNAL for pre/post insn callbacks that doesn't affect register allocation)
    int host_reg = ctz32(regs_free & ~ctx->callee_saved_regs);
    if (host_reg - reg_base >= 16) {
        host_reg = ctz32(regs_free);
    }
    if (host_reg - reg_base < 16) {
        ASSERT(!ctx->reg_map[host_reg]);
        reg_info->host_allocated = true;
        reg_info->host_reg = host_reg;
        ctx->reg_map[host_reg] = reg_index;
        ctx->regs_free ^= 1 << host_reg;
        ctx->regs_touched |= 1 << host_reg;
        return;
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
 */
static void allocate_regs_for_insn(HostX86Context *ctx, int insn_index)
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
            // FIXME: try to move shift counts into CL
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
        /* SSA implies that destination registers should never have a host
         * register allocated. */
        ASSERT(dest_reg->birth == insn_index);
        ASSERT(!dest_info->host_allocated);

        /* Special case for LOAD_ARG: try to reuse the same register the
         * argument is passed in. */
        // FIXME: only appropriate if no native calls
        if (insn->opcode == RTLOP_LOAD_ARG) {
            // FIXME: deal with not-in-register arguments more cleanly
            const X86Register target_reg =
                host_x86_int_arg_register(ctx, insn->arg_index);
            if (!ctx->reg_map[target_reg]) {
                dest_info->host_allocated = true;
                dest_info->host_reg = target_reg;
                ctx->reg_map[target_reg] = dest;
                ASSERT(ctx->regs_free & (1 << target_reg));
                ctx->regs_free ^= 1 << target_reg;
                ctx->regs_touched |= 1 << target_reg;
            }
        }

        /* x86 doesn't have separate destination operands for most
         * instructions, so if one of the source operands (if any) dies at
         * this instruction, reuse its host register for the destination
         * to avoid an unnecessary register move. */
        if (!dest_info->host_allocated
         && src1 && src1_reg->death == insn_index) {
            /* The first operand's register can always be reused for the
             * destination. */
            dest_info->host_allocated = true;
            dest_info->host_reg = src1_info->host_reg;
            ctx->reg_map[dest_info->host_reg] = dest;
        }
        if (!dest_info->host_allocated
         && src2 && src2_reg->death == insn_index) {
            /* The second operand's register can only be reused for
             * commutative operations. */
            static const uint8_t non_commutative[RTLOP__LAST / 8 + 1] = {
                /* Note that opcodes will need to be shifted around here
                 * if their numeric values are changed such that they move
                 * to different bytes. */
                0,
                1U<<(RTLOP_SUB-8) | 1U<<(RTLOP_DIVU-8) | 1U<<(RTLOP_DIVS-8),
                1U<<(RTLOP_MODU-16) | 1U<<(RTLOP_MODS-16)
                    | 1U<<(RTLOP_SLL-16) | 1U<<(RTLOP_SRL-16),
                1U<<(RTLOP_SRA-24) | 1U<<(RTLOP_ROR-24),
                /* SLT/SLE are technically non-commutative, but since it's
                 * a 2-instruction sequence (cmp/setCC) on x86 anyway, we
                 * don't have to worry about operand order. */
                1U<<(RTLOP_BFINS-32),
            };
            ASSERT(insn->opcode >= RTLOP__FIRST && insn->opcode <= RTLOP__LAST);
            if (!(non_commutative[insn->opcode/32] & (1<<(insn->opcode%32)))) {
                dest_info->host_allocated = true;
                dest_info->host_reg = src2_info->host_reg;
                ctx->reg_map[dest_info->host_reg] = dest;
            }
        }

        /* If none of the special cases apply, allocate a register normally. */
        if (!dest_info->host_allocated) {
            allocate_register(ctx, dest);
        }

        /* If the register isn't referenced again, immediately free it.
         * Normally such instructions will be optimized out, but if
         * optimization is disabled, for example, we shouldn't leave the
         * register allocated forever. */
        if (UNLIKELY(dest_reg->death == insn_index)) {
            ctx->regs_free |= 1 << dest_info->host_reg;
            ctx->reg_map[dest_info->host_reg] = 0;
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * allocate_regs_for_block:  Allocate host registers for RTL registers in
 * the given basic block.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 */
static void allocate_regs_for_block(HostX86Context *ctx, int block_index)
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
        allocate_regs_for_insn(ctx, insn_index);
    }
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

void host_x86_allocate_registers(HostX86Context *ctx)
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

    for (int block_index = 0; block_index >= 0;
         block_index = unit->blocks[block_index].next_block)
    {
        allocate_regs_for_block(ctx, block_index);
    }
}

/*-----------------------------------------------------------------------*/

X86Register host_x86_int_arg_register(HostX86Context *ctx, int index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(index >= 0);

    if (ctx->handle->setup.host == BINREC_ARCH_X86_64_SYSV) {
        ASSERT(index < 6);
        static const uint8_t regs[6] =
            {X86_DI, X86_SI, X86_DX, X86_CX, X86_R8, X86_R9};
        return regs[index];
    } else {
        ASSERT(ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS
            || ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);
        ASSERT(index < 4);
        static const uint8_t regs[6] = {X86_CX, X86_DX, X86_R8, X86_R9};
        return regs[index];
    }
}

/*************************************************************************/
/*************************************************************************/
