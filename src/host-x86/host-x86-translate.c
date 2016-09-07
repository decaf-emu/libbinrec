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
#include "src/endian.h"
#include "src/host-x86.h"
#include "src/host-x86/host-x86-internal.h"
#include "src/host-x86/host-x86-opcodes.h"
#include "src/rtl-internal.h"

/*************************************************************************/
/*************** Utility routines for adding instructions ****************/
/*************************************************************************/

/**
 * append_opcode:  Append an x86 opcode to the current code stream.  The
 * code buffer is assumed to have enough space for the instruction.
 */
static inline void append_opcode(binrec_t *handle, X86Opcode opcode)
{
    uint8_t *ptr = handle->code_buffer + handle->code_len;

    if (opcode <= 0xFF) {
        ASSERT(handle->code_len + 1 <= handle->code_buffer_size);
        handle->code_len += 1;
        *ptr++ = opcode;
    } else if (opcode <= 0xFFFF) {
        ASSERT(handle->code_len + 2 <= handle->code_buffer_size);
        handle->code_len += 2;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else if (opcode <= 0xFFFFFF) {
        ASSERT(handle->code_len + 3 <= handle->code_buffer_size);
        handle->code_len += 3;
        *ptr++ = opcode >> 16;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else {
        ASSERT(handle->code_len + 4 <= handle->code_buffer_size);
        handle->code_len += 4;
        *ptr++ = opcode >> 24;
        *ptr++ = opcode >> 16;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_imm8:  Append an 8-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_imm8(binrec_t *handle, uint8_t value)
{
    uint8_t *ptr = handle->code_buffer + handle->code_len;

    ASSERT(handle->code_len + 1 <= handle->code_buffer_size);
    handle->code_len += 1;
    *ptr++ = value;
}

/*-----------------------------------------------------------------------*/

/**
 * append_imm32:  Append a 32-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_imm32(binrec_t *handle, uint32_t value)
{
    uint8_t *ptr = handle->code_buffer + handle->code_len;

    ASSERT(handle->code_len + 4 <= handle->code_buffer_size);
    handle->code_len += 4;
    *ptr++ = (uint8_t)(value >>  0);
    *ptr++ = (uint8_t)(value >>  8);
    *ptr++ = (uint8_t)(value >> 16);
    *ptr++ = (uint8_t)(value >> 24);
}

/*-----------------------------------------------------------------------*/

/**
 * append_ModRM:  Append a ModR/M byte to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_ModRM(binrec_t *handle, X86Mod mod,
                                int reg_opcode, int r_m)
{
    uint8_t *ptr = handle->code_buffer + handle->code_len;

    ASSERT(handle->code_len + 1 <= handle->code_buffer_size);
    handle->code_len += 1;
    *ptr++ = x86_ModRM(mod, reg_opcode, r_m);
}

/*-----------------------------------------------------------------------*/

/**
 * append_ModRM:  Append a ModR/M and SIB byte pair to the current code
 * stream.  The code buffer is assumed to have enough space.
 */
static inline void append_ModRM_SIB(
    binrec_t *handle, X86Mod mod, int reg_opcode, int scale, int index,
    int base)
{
    uint8_t *ptr = handle->code_buffer + handle->code_len;

    ASSERT(handle->code_len + 2 <= handle->code_buffer_size);
    handle->code_len += 2;
    *ptr++ = x86_ModRM(mod, reg_opcode, X86MODRM_SIB);
    *ptr++ = x86_SIB(scale, index, base);
}

/*************************************************************************/
/*************************** Translation core ****************************/
/*************************************************************************/

/**
 * translate_block:  Translate the given RTL basic block.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 * [Return value]
 *     True on success, false on error.
 */
static bool translate_block(HostX86Context *ctx, int block_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(block_index >= 0);
    ASSERT(block_index < ctx->unit->num_blocks);

    const RTLUnit * const unit = ctx->unit;
    const RTLBlock * const block = &unit->blocks[block_index];
    HostX86BlockInfo * const block_info = &ctx->blocks[block_index];

    STATIC_ASSERT(sizeof(block_info->initial_reg_map) == sizeof(ctx->reg_map),
                  "Mismatched reg_map sizes");
    memcpy(ctx->reg_map, block_info->initial_reg_map, sizeof(ctx->reg_map));

    block_info->unresolved_branch_offset = -1;

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        const RTLInsn * const insn = &unit->insns[insn_index];
        ASSERT(insn->opcode >= RTLOP__FIRST);
        ASSERT(insn->opcode <= RTLOP__LAST);
        const uint32_t dest = insn->dest;
        UNUSED const uint32_t src1 = insn->src1; //FIXME: not yet used
        UNUSED const uint32_t src2 = insn->src2; //FIXME: not yet used

        /* No translations need more than 64 bytes. */
        if (UNLIKELY(!binrec_ensure_code_space(ctx->handle, 64))) {
            return false;
        }

        switch (insn->opcode) {

          case RTLOP_NOP:
            if (insn->src_imm != 0) {
                append_opcode(ctx->handle, X86OP_NOP_Ev);
                append_ModRM(ctx->handle, X86MOD_DISP0, 0, X86MODRM_RIP_REL);
                append_imm32(ctx->handle, (uint32_t)insn->src_imm);
                if (insn->src_imm >> 32) {
                    append_opcode(ctx->handle, X86OP_NOP_Ev);
                    append_ModRM_SIB(ctx->handle, X86MOD_DISP32, 0,
                                     0, X86SIB_NOINDEX, X86_SP);
                    append_imm32(ctx->handle, (uint32_t)(insn->src_imm >> 32));
                }
            }
            break;

          case RTLOP_LOAD_IMM: {
            const uint64_t imm = insn->src_imm;
            const X86Register host_dest = ctx->regs[dest].host_reg;
            switch (unit->regs[dest].type) {
              case RTLTYPE_INT32:
              case RTLTYPE_ADDRESS:
                if (imm == 0) {
                    if (host_dest & 8) {
                        append_opcode(ctx->handle, X86OP_REX_R | X86OP_REX_B);
                    }
                    append_opcode(ctx->handle, X86OP_XOR_Gv_Ev);
                    append_ModRM(ctx->handle, X86MOD_REG, host_dest, host_dest);
                } else if (imm <= UINT64_C(0xFFFFFFFF)) {
                    if (host_dest & 8) {
                        append_opcode(ctx->handle, X86OP_REX_B);
                    }
                    append_opcode(ctx->handle,
                                  X86OP_MOV_rAX_Iv | (host_dest & 7));
                    append_imm32(ctx->handle, (uint32_t)imm);
                } else if (imm >= UINT64_C(0xFFFFFFFF80000000)) {
                    append_opcode(ctx->handle, (host_dest & 8
                                                ? X86OP_REX_W | X86OP_REX_B
                                                : X86OP_REX_W));
                    append_opcode(ctx->handle, X86OP_MOV_Ev_Iz);
                    append_ModRM(ctx->handle, X86MOD_REG, host_dest, host_dest);
                    append_imm32(ctx->handle, (uint32_t)imm);
                } else {
                    append_opcode(ctx->handle, (host_dest & 8
                                                ? X86OP_REX_W | X86OP_REX_B
                                                : X86OP_REX_W));
                    append_opcode(ctx->handle,
                                  X86OP_MOV_rAX_Iv | (host_dest & 7));
                    append_imm32(ctx->handle, (uint32_t)imm);
                    append_imm32(ctx->handle, (uint32_t)(imm >> 32));
                }
                break;
              default:
                // FIXME: handle FP registers when we support those
                ASSERT(!"Invalid data type in LOAD_IMM");
            }
            break;
          }  // case RTLOP_LOAD_IMM

          case RTLOP_ILLEGAL:
            append_opcode(ctx->handle, X86OP_UD2);
            break;

          default: break; //FIXME: other insns not yet implemented

        }
    }

    return true;
}

/*************************************************************************/
/************************* Other local routines **************************/
/*************************************************************************/

/**
 * append_prologue:  Append the function prologue to the output code buffer.
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     True on success, false on error.
 */
static bool append_prologue(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->frame_size % 16 == 0);

    binrec_t * const handle = ctx->handle;
    const uint32_t regs_to_save =
        ctx->regs_touched & host_x86_callee_saved_registers(ctx);
    const bool is_windows_seh =
        (ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);

    /* Figure out how much stack space we use in total. */
    int total_stack_use;
    const int push_size = 8 * popcnt32(regs_to_save & 0xFFFF);
    total_stack_use = push_size;
    /* We have to realign the stack at this specific point; if we're saving
     * XMM registers then those have to be aligned, and in any case we
     * maintain our local stack space multiples of 16 bytes.  Somewhat
     * confusingly, this means setting total_stack_use to an _un_aligned
     * value, since the stack pointer comes in unaligned due to the return
     * address that was just pushed onto it. */
    if (total_stack_use % 16 == 0) {
        total_stack_use += 8;
    }
    total_stack_use += 16 * popcnt32(regs_to_save >> 16) + ctx->frame_size;

    /* Calculate the amount of stack space to reserve, excluding GPR pushes. */
    const int stack_alloc = total_stack_use - push_size;
    ctx->stack_alloc = stack_alloc;

    if (is_windows_seh) {
        /* Create unwind data for the function, because Microsoft likes
         * finding ways to make everybody's lives harder... */

        enum {
            UWOP_PUSH_NONVOL = 0,
            UWOP_ALLOC_LARGE = 2,
            UWOP_ALLOC_SMALL = 2,
            UWOP_SAVE_XMM128 = 8,
        };

        /* We'll have at most:
         *    1 * 8 GPRs
         *    2 * stack adjustment
         *    2 * 10 XMM registers
         * for a total of 30 16-bit data words. */
        uint8_t unwind_info[4 + 2*30];

        int prologue_pos = 0;

        unwind_info[0] = 1;  // version:3, flags:5
        unwind_info[1] = 0;  // Size of prologue (will be filled in later)
        unwind_info[2] = 0;  // Code count (will be filled in later)
        unwind_info[3] = 0;  // Frame register/offset (not used)

        /* The unwind information has to be given in reverse order (?!),
         * so generate from the end of the buffer and move it into place
         * when we're done. */
        int unwind_pos = sizeof(unwind_info);

        for (int reg = 0; reg < 16; reg++) {
            if (regs_to_save & (1 << reg)) {
                unwind_pos -= 2;
                ASSERT(unwind_pos >= 4);
                unwind_info[unwind_pos+0] = prologue_pos;
                unwind_info[unwind_pos+1] = UWOP_PUSH_NONVOL | reg<<4;
                if (reg < 8) {
                    prologue_pos += 1;  // PUSH
                } else {
                    prologue_pos += 2;  // REX PUSH
                }
            }
        }

        if (stack_alloc > 0) {
            const int stack_alloc_info = (stack_alloc / 8) - 1;
            if (stack_alloc > 128) {
                ASSERT(stack_alloc < 524288);  // Should never need this much.
                unwind_pos -= 4;
                ASSERT(unwind_pos >= 4);
                unwind_info[unwind_pos+0] = prologue_pos;
                unwind_info[unwind_pos+1] = UWOP_ALLOC_LARGE;
                unwind_info[unwind_pos+2] = (uint8_t)(stack_alloc_info >> 0);
                unwind_info[unwind_pos+3] = (uint8_t)(stack_alloc_info >> 8);
                prologue_pos += 7;
            } else {
                unwind_pos -= 2;
                ASSERT(unwind_pos >= 4);
                unwind_info[unwind_pos+0] = prologue_pos;
                unwind_info[unwind_pos+1] =
                    UWOP_ALLOC_SMALL | stack_alloc_info << 4;
                if (stack_alloc == 128) {
                    prologue_pos += 7;
                } else {
                    prologue_pos += 4;
                }
            }
        }

        for (int reg = 16; reg < 32; reg++) {
            int sp_offset = ctx->frame_size;
            if (regs_to_save & (1 << reg)) {
                unwind_pos -= 4;
                ASSERT(unwind_pos >= 4);
                unwind_info[unwind_pos+0] = prologue_pos;
                unwind_info[unwind_pos+1] = UWOP_SAVE_XMM128 | reg<<4;
                unwind_info[unwind_pos+2] = (uint8_t)(sp_offset >> 0);
                unwind_info[unwind_pos+3] = (uint8_t)(sp_offset >> 8);
                if (reg < 8) {
                    prologue_pos += 3;  // MOVAPS
                } else {
                    prologue_pos += 4;  // REX MOVAPS
                }
                if (sp_offset >= 128) {
                    prologue_pos += 4;  // disp32
                } else if (sp_offset > 0) {
                    prologue_pos += 1;  // disp8
                }
                sp_offset += 16;
            }
        }

        const int code_size = sizeof(unwind_info) - unwind_pos;
        memmove(unwind_info + 4, unwind_info + unwind_pos, code_size);
        const int size = 4 + code_size;

        unwind_info[1] = prologue_pos;
        ASSERT(size <= (int)sizeof(unwind_info));
        unwind_info[2] = code_size / 2;

        ASSERT(ctx->handle->code_alignment >= 8);
        int code_offset = 8 + size;
        if (code_offset % ctx->handle->code_alignment != 0) {
            code_offset += ctx->handle->code_alignment - (code_offset % ctx->handle->code_alignment);
        }
        if (UNLIKELY(!binrec_ensure_code_space(ctx->handle, code_offset))) {
            return false;
        }

        uint8_t *buffer = ctx->handle->code_buffer;
        *ALIGNED_CAST(uint64_t *, buffer) = bswap_le64(code_offset);
        memcpy(buffer + 8, unwind_info, size);
        ASSERT(code_offset >= 8+size);
        memset(buffer + (8+size), 0, code_offset - (8+size));
        ctx->handle->code_len += code_offset;
    }

    /* In the worst case, the prologue will require:
     *    1 * 4 low GPR saves
     *    2 * 4 high GPR saves
     *    7 * 1 stack adjustment
     *    7 * 2 low XMM saves
     *    8 * 8 high XMM saves
     * for a total of 97 bytes. */
    if (UNLIKELY(!binrec_ensure_code_space(ctx->handle, 97))) {
        return false;
    }

    for (int reg = 0; reg < 16; reg++) {
        if (regs_to_save & (1 << reg)) {
            if (reg & 8) {
                append_opcode(handle, X86OP_REX_B);
            }
            append_opcode(handle, X86OP_PUSH_rAX | (reg & 7));
        }
    }

    if (stack_alloc >= 128) {
        append_opcode(handle, X86OP_REX_W);
        append_opcode(handle, X86OP_IMM_Ev_Iz);
        append_ModRM(handle, X86MOD_REG, X86OP_IMM_SUB, X86_SP);
        append_imm32(handle, stack_alloc);
    } else if (stack_alloc > 0) {
        append_opcode(handle, X86OP_REX_W);
        append_opcode(handle, X86OP_IMM_Ev_Ib);
        append_ModRM(handle, X86MOD_REG, X86OP_IMM_SUB, X86_SP);
        append_imm8(handle, stack_alloc);
    }

    int sp_offset = ctx->frame_size;
    for (int reg = 16; reg < 32; reg++) {
        if (regs_to_save & (1 << reg)) {
            if (reg & 8) {
                append_opcode(handle, X86OP_REX_R);
            }
            append_opcode(handle, X86OP_MOVAPS_W_V);
            if (sp_offset >= 128) {
                append_ModRM_SIB(handle, X86MOD_DISP32, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm32(handle, sp_offset);
            } else if (sp_offset > 0) {
                append_ModRM_SIB(handle, X86MOD_DISP8, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm8(handle, sp_offset);
            } else {
                append_ModRM_SIB(handle, X86MOD_DISP0, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
            }
            sp_offset += 16;
        }
    }

    if (is_windows_seh) {
        /* Make sure the prologue is the same length we said it would be. */
        const int code_offset =
            (int) *ALIGNED_CAST(uint64_t *, ctx->handle->code_buffer);
        ASSERT(ctx->handle->code_len ==
               code_offset + ctx->handle->code_buffer[9]);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * append_epilogue:  Append the function epilogue to the output code buffer.
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     True on success, false on error.
 */
static bool append_epilogue(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);

    binrec_t * const handle = ctx->handle;
    const uint32_t regs_saved =
        ctx->regs_touched & host_x86_callee_saved_registers(ctx);
    const int stack_alloc = ctx->stack_alloc;

    ctx->label_offsets[0] = handle->code_len;

    int sp_offset = ctx->frame_size + 16 * popcnt32(regs_saved >> 16);
    for (int reg = 31; reg >= 16; reg--) {
        if (regs_saved & (1 << reg)) {
            sp_offset -= 16;
            if (reg & 8) {
                append_opcode(handle, X86OP_REX_R);
            }
            append_opcode(handle, X86OP_MOVAPS_V_W);
            if (sp_offset >= 128) {
                append_ModRM_SIB(handle, X86MOD_DISP32, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm32(handle, sp_offset);
            } else if (sp_offset > 0) {
                append_ModRM_SIB(handle, X86MOD_DISP8, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm8(handle, sp_offset);
            } else {
                append_ModRM_SIB(handle, X86MOD_DISP0, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
            }
        }
    }

    if (stack_alloc >= 128) {
        append_opcode(handle, X86OP_REX_W);
        append_opcode(handle, X86OP_IMM_Ev_Iz);
        append_ModRM(handle, X86MOD_REG, X86OP_IMM_ADD, X86_SP);
        append_imm32(handle, stack_alloc);
    } else if (stack_alloc > 0) {
        append_opcode(handle, X86OP_REX_W);
        append_opcode(handle, X86OP_IMM_Ev_Ib);
        append_ModRM(handle, X86MOD_REG, X86OP_IMM_ADD, X86_SP);
        append_imm8(handle, stack_alloc);
    }

    for (int reg = 15; reg >= 0; reg--) {
        if (regs_saved & (1 << reg)) {
            if (reg & 8) {
                append_opcode(handle, X86OP_REX_B);
            }
            append_opcode(handle, X86OP_POP_rAX | (reg & 7));
        }
    }

    if (UNLIKELY(!binrec_ensure_code_space(handle, 1))) {
        return false;
    }
    append_opcode(handle, X86OP_RET);

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * resolve_branches:  Resolve forward branches in the generated code.
 *
 * When generating a forward branch, we don't yet know what the offset to
 * the target instruction will be, so we use this function to fill it in
 * after code generation is complete.
 *
 * FIXME: shrink 32 bit offset -> 8 bit offset if possible; may have a recursive effect on surrounding branches as well
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     True on success, false on error.
 */
static bool resolve_branches(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);

    // FIXME: not yet implemented

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * translate_unit:  Translate the RTLUnit associated with the given
 * translation context.
 *
 * [Parameters]
 *     ctx: Translation context.
 * [Return value]
 *     True on success, false on error.
 */
static bool translate_unit(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    // FIXME: calculate frame size when we have something to put on the stack

    if (!append_prologue(ctx)) {
        return false;
    }

    const RTLUnit * const unit = ctx->unit;
    for (int i = 0; i >= 0; i = unit->blocks[i].next_block) {
        if (!translate_block(ctx, i)) {
            return false;
        }
    }

    if (!append_epilogue(ctx)) {
        return false;
    }

    if (!resolve_branches(ctx)) {
        return false;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * destroy_context:  Free all resources used by the given context.  The
 * context is assumed to have been initialized.
 *
 * [Parameters]
 *     ctx: Context to clear.
 */
static void destroy_context(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);

    binrec_free(ctx->handle, ctx->blocks);
    binrec_free(ctx->handle, ctx->regs);
    binrec_free(ctx->handle, ctx->label_offsets);
}

/*-----------------------------------------------------------------------*/

/**
 * init_context:  Set up the given context for translation.
 *
 * [Parameters]
 *     ctx: Context to initialize.
 *     handle: Translation handle.
 *     unit: RTLUnit to be translated.
 * [Return value]
 *     True on success, false on error.
 */
static bool init_context(HostX86Context *ctx, binrec_t *handle, RTLUnit *unit)
{
    ASSERT(ctx);

    memset(ctx, 0, sizeof(*ctx));
    ctx->handle = handle;
    ctx->unit = unit;

    ctx->blocks = binrec_malloc(
        handle, sizeof(*ctx->blocks) * unit->num_blocks);
    ctx->regs = binrec_malloc(handle, sizeof(*ctx->regs) * unit->next_reg);
    ctx->label_offsets = binrec_malloc(
        handle, sizeof(*ctx->label_offsets) * unit->next_label);
    if (!ctx->blocks || !ctx->regs || !ctx->label_offsets) {
        log_error(handle, "No memory for output translation context");
        destroy_context(ctx);
        return false;
    }

    memset(ctx->blocks, 0, sizeof(*ctx->blocks) * unit->num_blocks);
    memset(ctx->regs, 0, sizeof(*ctx->regs) * unit->next_reg);
    memset(ctx->label_offsets, 0,
           sizeof(*ctx->label_offsets) * unit->next_label);

    return true;
}

/*************************************************************************/
/************************ Translation entry point ************************/
/*************************************************************************/

bool host_x86_translate(binrec_t *handle, struct RTLUnit *unit)
{
    ASSERT(handle);
    ASSERT(unit);

    if (!unit->num_blocks || !unit->num_insns) {
        log_error(handle, "No code to translate");
        goto error_return;
    }

    HostX86Context ctx;
    if (!init_context(&ctx, handle, unit)) {
        goto error_return;
    }

    if (!host_x86_allocate_registers(&ctx)) {
        goto error_destroy_context;
    }

    if (!translate_unit(&ctx)) {
        goto error_destroy_context;
    }

    destroy_context(&ctx);
    return true;

  error_destroy_context:
    destroy_context(&ctx);
  error_return:
    return false;
}

/*************************************************************************/
/*************************************************************************/
