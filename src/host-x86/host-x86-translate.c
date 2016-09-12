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
/******************* Local data structure definitions ********************/
/*************************************************************************/

/**
 * CodeBuffer:  Structure encapsulating an output code buffer and its
 * allocated size and current length.  Used to help optimization by
 * letting the compiler know it doesn't have to write size data back to
 * the handle every few bytes.
 */
typedef struct CodeBuffer {
    uint8_t * restrict buffer;
    long buffer_size;
    long len;
} CodeBuffer;

/*************************************************************************/
/*************** Utility routines for adding instructions ****************/
/*************************************************************************/

/*
 * For ordinary compilation, it's critical to have all these functions
 * inlined so the compiler knows that local variables (including the
 * CodeBuffer structure) are not clobbered by writes to the output code
 * buffer; this can reduce instruction count in the translation loop by
 * around 30% (observed with GCC 5.3) if the compiler would ordinarily
 * choose not to inline the functions.  For coverage testing, however,
 * it's more useful to see the branch coverage of the functions themselves
 * rather than of every location they're used in the code.
 */
#ifdef COVERAGE
    #define APPEND_INLINE  /*nothing*/
#else
    #define APPEND_INLINE  ALWAYS_INLINE
#endif

/**
 * append_opcode:  Append an x86 opcode to the current code stream.  The
 * code buffer is assumed to have enough space for the instruction.
 */
static APPEND_INLINE void append_opcode(CodeBuffer *code, X86Opcode opcode)
{
    uint8_t *ptr = code->buffer + code->len;

    if (opcode <= 0xFF) {
        ASSERT(code->len + 1 <= code->buffer_size);
        code->len += 1;
        *ptr++ = opcode;
    } else if (opcode <= 0xFFFF) {
        ASSERT(code->len + 2 <= code->buffer_size);
        code->len += 2;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else if (opcode <= 0xFFFFFF) {
        ASSERT(code->len + 3 <= code->buffer_size);
        code->len += 3;
        *ptr++ = opcode >> 16;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else {
        ASSERT(code->len + 4 <= code->buffer_size);
        code->len += 4;
        *ptr++ = opcode >> 24;
        *ptr++ = opcode >> 16;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_rex_opcode:  Append an x86 opcode with REX prefix to the current
 * code stream.  The code buffer is assumed to have enough space for the
 * instruction.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     rex: REX flags (bitwise OR of X86_REX_* or X86OP_REX_*).
 *     opcode: Opcode to append.
 */
static APPEND_INLINE void append_rex_opcode(CodeBuffer *code, uint8_t rex,
                                            X86Opcode opcode)
{
    uint8_t *ptr = code->buffer + code->len;
    rex |= X86OP_REX;

    if (opcode <= 0xFF) {
        ASSERT(code->len + 2 <= code->buffer_size);
        code->len += 2;
        *ptr++ = rex;
        *ptr++ = opcode;
    } else if (opcode <= 0xFFFF) {
        ASSERT(code->len + 3 <= code->buffer_size);
        code->len += 3;
        *ptr++ = rex;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else if (opcode <= 0xFFFFFF) {
        ASSERT(code->len + 4 <= code->buffer_size);
        code->len += 4;
        if (opcode>>16 == 0x66 || opcode>>16 == 0xF2 || opcode>>16 == 0xF3) {
            *ptr++ = opcode >> 16;
            *ptr++ = rex;
        } else {
            *ptr++ = rex;
            *ptr++ = opcode >> 16;
        }
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else {
        ASSERT(code->len + 5 <= code->buffer_size);
        code->len += 5;
        if (opcode>>24 == 0x66 || opcode>>24 == 0xF2 || opcode>>24 == 0xF3) {
            *ptr++ = opcode >> 24;
            *ptr++ = rex;
        } else {
            *ptr++ = rex;
            *ptr++ = opcode >> 24;
        }
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
static APPEND_INLINE void append_imm8(CodeBuffer *code, uint8_t value)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 1 <= code->buffer_size);
    code->len += 1;
    *ptr++ = value;
}

/*-----------------------------------------------------------------------*/

/**
 * append_imm32:  Append a 32-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static APPEND_INLINE void append_imm32(CodeBuffer *code, uint32_t value)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 4 <= code->buffer_size);
    code->len += 4;
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
static APPEND_INLINE void append_ModRM(CodeBuffer *code, X86Mod mod,
                                       int reg_opcode, int r_m)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 1 <= code->buffer_size);
    code->len += 1;
    *ptr++ = x86_ModRM(mod, reg_opcode, r_m);
}

/*-----------------------------------------------------------------------*/

/**
 * append_ModRM:  Append a ModR/M and SIB byte pair to the current code
 * stream.  The code buffer is assumed to have enough space.
 */
static APPEND_INLINE void append_ModRM_SIB(
    CodeBuffer *code, X86Mod mod, int reg_opcode, int scale, int index,
    int base)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 2 <= code->buffer_size);
    code->len += 2;
    *ptr++ = x86_ModRM(mod, reg_opcode, X86MODRM_SIB);
    *ptr++ = x86_SIB(scale, index, base);
}

/*-----------------------------------------------------------------------*/

/**
 * append_test_reg:  Append an instruction to test the value of the given
 * RTL register.
 */
static inline void append_test_reg(HostX86Context *ctx, CodeBuffer *code,
                                   uint32_t reg)
{
    if ((ctx->handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES)
     && ctx->cc_reg == reg) {
        return;  // Condition codes are already set appropriately.
    }

    const X86Register host_reg = ctx->regs[reg].host_reg;
    uint8_t rex = 0;
    if (ctx->unit->regs[reg].type == RTLTYPE_ADDRESS) {
        rex |= X86_REX_W;
    }
    if (host_reg & 8) {
        rex |= X86_REX_R | X86_REX_B;
    }
    if (rex) {
        append_rex_opcode(code, rex, X86OP_TEST_Ev_Gv);
    } else {
        append_opcode(code, X86OP_TEST_Ev_Gv);
    }
    append_ModRM(code, X86MOD_REG, host_reg & 7, host_reg & 7);

    ctx->cc_reg = reg;
}

/*-----------------------------------------------------------------------*/

/**
 * append_insn_R:  Append an instruction which takes a single register
 * operand encoded in the opcode itself (such as PUSH).
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True for 64-bit operands, false for 32-bit operands.
 *     opcode: Instruction opcode.
 *     reg: Register index.
 */
static APPEND_INLINE void append_insn_R(
    CodeBuffer *code, bool is64, X86Opcode opcode, X86Register reg)
{
    uint8_t rex = is64 ? X86_REX_W : 0;
    if (reg & 8) {
        rex |= X86_REX_B;
    }
    if (rex) {
        append_rex_opcode(code, rex, opcode | (reg & 7));
    } else {
        append_opcode(code, opcode | (reg & 7));
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_insn_ModRM_reg:  Append an instruction which takes a ModR/M byte,
 * encoding a register EA in the instruction.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True for 64-bit operands, false for 32-bit operands.
 *     opcode: Instruction opcode.
 *     reg1: Register index (or sub-opcode) for ModR/M reg field.
 *     reg2: Register index for ModR/M r/m field.
 */
static APPEND_INLINE void append_insn_ModRM_reg(
    CodeBuffer *code, bool is64, X86Opcode opcode, X86Register reg1,
    X86Register reg2)
{
    uint8_t rex = is64 ? X86_REX_W : 0;
    if (reg1 & 8) {
        rex |= X86_REX_R;
    }
    if (reg2 & 8) {
        rex |= X86_REX_B;
    }
    if (rex) {
        append_rex_opcode(code, rex, opcode);
    } else {
        append_opcode(code, opcode);
    }
    append_ModRM(code, X86MOD_REG, reg1 & 7, reg2 & 7);
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

    binrec_t * const handle = ctx->handle;
    const RTLUnit * const unit = ctx->unit;
    const RTLBlock * const block = &unit->blocks[block_index];
    HostX86BlockInfo * const block_info = &ctx->blocks[block_index];

    CodeBuffer code = {.buffer = handle->code_buffer,
                       .buffer_size = handle->code_buffer_size,
                       .len = handle->code_len};

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
        const uint32_t src1 = insn->src1;
        const uint32_t src2 = insn->src2;

        /* No instruction translations need more than 28 bytes.  (Worst
         * case: BFINS with 64-bit src1 and 32-bit src2 masks.) */
        const int MAX_INSN_LEN = 28;
        if (UNLIKELY(code.len + MAX_INSN_LEN > code.buffer_size)) {
            handle->code_len = code.len;
            if (UNLIKELY(!binrec_ensure_code_space(handle, MAX_INSN_LEN))) {
                return false;
            }
            code.buffer = handle->code_buffer;
            code.buffer_size = handle->code_buffer_size;
        }

        const long initial_len = code.len;

        switch (insn->opcode) {

          case RTLOP_NOP:
            if (insn->src_imm != 0) {
                append_opcode(&code, X86OP_NOP_Ev);
                append_ModRM(&code, X86MOD_DISP0, 0, X86MODRM_RIP_REL);
                append_imm32(&code, (uint32_t)insn->src_imm);
                if (insn->src_imm >> 32) {
                    append_opcode(&code, X86OP_NOP_Ev);
                    append_ModRM_SIB(&code, X86MOD_DISP32, 0,
                                     0, X86SIB_NOINDEX, X86_SP);
                    append_imm32(&code, (uint32_t)(insn->src_imm >> 32));
                }
            }
            break;

          case RTLOP_MOVE: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            if (host_dest != host_src1) {
                const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src1);
            }
            break;
          }  // case RTLOP_MOVE

          case RTLOP_SELECT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);

            /* Set the condition codes based on the condition register. */
            append_test_reg(ctx, &code, insn->cond);

            /* Put one of the source values in the destination register, if
             * necessary.  Note that MOV does not alter flags. */
            bool dest_is_src1;
            if (host_dest == host_src1) {
                dest_is_src1 = true;
            } else if (host_dest == host_src2) {
                dest_is_src1 = false;
            } else {
                dest_is_src1 = true;
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src1);
            }

            /* Conditionally move the other value into the register. */
            if (dest_is_src1) {
                append_insn_ModRM_reg(&code, is64, X86OP_CMOVZ,
                                      host_dest, host_src2);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_CMOVNZ,
                                      host_dest, host_src1);
            }
            break;
          }  // case RTLOP_SELECT

          case RTLOP_SCAST:
          case RTLOP_ZCAST: {
            const RTLDataType type_dest = unit->regs[dest].type;
            const RTLDataType type_src1 = unit->regs[src1].type;
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            if (type_dest == RTLTYPE_ADDRESS) {
                if (type_src1 == RTLTYPE_ADDRESS) {
                    if (host_dest != host_src1) {
                        append_insn_ModRM_reg(&code, true, X86OP_MOV_Ev_Gv,
                                              host_src1, host_dest);
                    }
                } else {
                    if (insn->opcode == RTLOP_SCAST) {
                        append_insn_ModRM_reg(&code, true, X86OP_MOVSXD_Gv_Ev,
                                              host_dest, host_src1);
                    } else {
                        append_insn_ModRM_reg(&code, false, X86OP_MOV_Ev_Gv,
                                              host_src1, host_dest);
                    }
                }
            } else {
                if (host_dest != host_src1) {
                    append_insn_ModRM_reg(&code, false, X86OP_MOV_Ev_Gv,
                                          host_src1, host_dest);
                }
            }
            break;
          }  // case RTLOP_SCAST, RTLOP_ZCAST

          case RTLOP_NEG:
          case RTLOP_NOT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            if (host_dest != host_src1) {
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Ev_Gv,
                                      host_src1, host_dest);
            }
            const X86UnaryOpcode opcode =
                insn->opcode == RTLOP_NOT ? X86OP_UNARY_NOT : X86OP_UNARY_NEG;
            append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev,
                                  opcode, host_dest);
            break;
          }  // case RTLOP_NEG, RTLOP_NOT

          case RTLOP_ADD:
          case RTLOP_SUB:
          case RTLOP_AND:
          case RTLOP_OR:
          case RTLOP_XOR: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_ADD ? X86OP_ADD_Gv_Ev :
                insn->opcode == RTLOP_SUB ? X86OP_SUB_Gv_Ev :
                insn->opcode == RTLOP_AND ? X86OP_AND_Gv_Ev :
                insn->opcode == RTLOP_OR ? X86OP_OR_Gv_Ev :
                             /* RTLOP_XOR */ X86OP_XOR_Gv_Ev);
            if (host_dest == host_src2) {
                append_insn_ModRM_reg(&code, is64, opcode,
                                      host_dest, host_src1);
            } else {
                if (host_dest != host_src1) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, host_src1);
                }
                append_insn_ModRM_reg(&code, is64, opcode,
                                      host_dest, host_src2);
            }
            break;
          }  // case RTLOP_{ADD,SUB,AND,OR,XOR}

          case RTLOP_MUL: {
            /* This case is identical to RTLOP_ADD (etc.), but it's
             * separated out to aid optimization, since the x86 IMUL
             * instruction is two bytes (0F AF) as opposed to the other
             * ALU instructions which are one byte. */
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = X86OP_IMUL_Gv_Ev;
            if (host_dest == host_src2) {
                append_insn_ModRM_reg(&code, is64, opcode,
                                      host_dest, host_src1);
            } else {
                if (host_dest != host_src1) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, host_src1);
                }
                append_insn_ModRM_reg(&code, is64, opcode,
                                      host_dest, host_src2);
            }
            break;
          }  // case RTLOP_MUL

          case RTLOP_MULHU:
          case RTLOP_MULHS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);

            /* The destination register will always get rDX if it's free,
             * so if the destination is somewhere else, we need to save
             * away the current value of RDX (all 64 bits, as usual). */
            bool swapped_dx = false;
            if (host_dest != X86_DX) {
                /* If dest shares a hardware register with src1 or src2,
                 * we need to preserve its value until the actual multiply;
                 * otherwise, we can use a MOV for potentially less latency. */
                if (host_dest == host_src1 || host_dest == host_src2) {
                    swapped_dx = true;
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          host_dest, X86_DX);
                } else {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          host_dest, X86_DX);
                }
            }
            /* The register allocator gives us a temporary iff rAX is live
             * across this instruction. */
            if (ctx->regs[dest].temp_allocated) {
                ASSERT(ctx->regs[dest].host_temp != X86_DX);
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      ctx->regs[dest].host_temp, X86_AX);
            }

            X86Register multiplier;
            if (host_src2 == X86_AX) {
                multiplier = host_src1;
            } else {
                if (host_src1 != X86_AX) {
                    /* Watch out for the input operands being moved around
                     * by XCHG! */
                    X86Register multiplicand = host_src1;
                    if (swapped_dx) {
                        if (multiplicand == X86_DX) {
                            multiplicand = host_dest;
                        } else if (multiplicand == host_dest) {
                            multiplicand = X86_DX;
                        }
                    }
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          X86_AX, multiplicand);
                }
                multiplier = host_src2;
            }
            if (swapped_dx) {
                if (multiplier == X86_DX) {
                    multiplier = host_dest;
                } else if (multiplier == host_dest) {
                    multiplier = X86_DX;
                }
            }

            const X86UnaryOpcode opcode = (
                insn->opcode == RTLOP_MULHU ? X86OP_UNARY_MUL_rAX:
                             /* RTLOP_MULHS */ X86OP_UNARY_IMUL_rAX);
            append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev, opcode,
                                  multiplier);

            if (host_dest != X86_DX) {
                /* This is always an XCHG, since both dest and the former
                 * rDX now have live values. */
                append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                      X86_DX, host_dest);
            }
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_AX, ctx->regs[dest].host_temp);
            }
            break;
          }  // case RTLOP_MULH[US]

          case RTLOP_SLL:
          case RTLOP_SRL:
          case RTLOP_SRA:
          case RTLOP_ROR: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            ASSERT(host_src2 != host_dest);
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_SLL ? X86OP_SHIFT_SHL :
                insn->opcode == RTLOP_SRL ? X86OP_SHIFT_SHR :
                insn->opcode == RTLOP_SRA ? X86OP_SHIFT_SAR :
                /* RTLOP_ROR */ X86OP_SHIFT_ROR);

            if (host_dest != host_src1) {
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src1);
            }

            /* If we couldn't allocate rCX for the second operand, swap
             * it with whatever's in there now.  We don't know what size
             * the value in rCX is, so always exchange in 64-bit to be safe.
             * This has to come after the src1->dest copy! */
            if (host_src2 != X86_CX) {
                append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                      X86_CX, host_src2);
            }

            /* If dest was in rCX, it's now in src2, so be sure to shift
             * the proper register. */
            if (host_dest == X86_CX) {
                append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_CL,
                                      opcode, host_src2);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_CL,
                                      opcode, host_dest);
            }

            /* If we had to swap rCX, restore the original register values.
             * But prefer MOV (and discard src2) if src2 dies on this
             * instruction, since MOV can be zero-latency. */
            if (host_src2 != X86_CX) {
                if (unit->regs[src2].death == insn_index) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          X86_CX, host_src2);
                } else {
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          X86_CX, host_src2);
                }
            }
            break;
          }  // case RTLOP_{SLL,SRL,SRA,ROR}

          case RTLOP_CLZ: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            if (handle->setup.host_features & BINREC_FEATURE_X86_LZCNT) {
                append_insn_ModRM_reg(&code, is64, X86OP_LZCNT,
                                      host_dest, host_src1);
            } else {
                ASSERT(ctx->regs[dest].temp_allocated);
                const X86Register host_temp = ctx->regs[dest].host_temp;
                append_insn_ModRM_reg(&code, is64, X86OP_BSR,
                                      host_dest, host_src1);
                append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_temp);
                append_imm32(&code, 32);
                append_insn_ModRM_reg(&code, is64, X86OP_CMOVZ,
                                      host_dest, host_temp);
            }
            break;
          }  // case RTLOP_CLZ

          case RTLOP_BSWAP: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            if (host_dest != host_src1) {
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Ev_Gv,
                                      host_src1, host_dest);
            }
            append_insn_R(&code, is64, X86OP_BSWAP_rAX, host_dest);
            break;
          }  // case RTLOP_BSWAP

          case RTLOP_SEQ:
          case RTLOP_SLTU:
          case RTLOP_SLTS:
          case RTLOP_SLEU:
          case RTLOP_SLES: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode set_opcode = (
                insn->opcode == RTLOP_SLTU ? X86OP_SETB :
                insn->opcode == RTLOP_SLTS ? X86OP_SETL :
                insn->opcode == RTLOP_SLEU ? X86OP_SETBE :
                insn->opcode == RTLOP_SLES ? X86OP_SETLE :
                /* RTLOP_SEQ */ X86OP_SETZ);
            append_insn_ModRM_reg(&code, is64, X86OP_CMP_Gv_Ev,
                                  host_src1, host_src2);
            /* Registers SP-DI require a REX prefix (even if empty) to
             * access the low byte as a byte register. */
            if (host_dest >= X86_SP && host_dest <= X86_DI) {
                append_opcode(&code, X86OP_REX);
            }
            append_insn_ModRM_reg(&code, false, set_opcode, 0, host_dest);
            if (host_dest >= X86_SP && host_dest <= X86_DI) {
                append_opcode(&code, X86OP_REX);
            }
            append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Eb,
                                  host_dest, host_dest);
            break;
          }  // case RTLOP_{SEQ,SLTU,SLTS,SLEU,SLES}

          case RTLOP_BFEXT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);

            /* BEXTR (from BMI2) is another option for implementing this
             * instruction, but it takes the source and count from a GPR
             * rather than encoding them in the instruction (presumably
             * due to ISA limitations), so we need an extra instruction
             * to load the shift/count and thus probably won't save any
             * time on average.  If anything, BEXTR has higher latency
             * than SHR and AND, so if we can omit one of the two (as when
             * extracting from one end of the register) we can actually
             * save time by not using BEXTR. */

            X86Register host_shifted;
            if (insn->bitfield.start != 0) {
                /* We could use SHRD (which takes separate input and output
                 * operands) to avoid the extra MOV, but SHRD has higher
                 * latency than SHR on current x86 architecture
                 * implementations, and simple inter-GPR moves are free
                 * (zero latency) on at least current Intel CPUs, so the
                 * only penalty is the time to decode the additional
                 * instruction.  Since we need the shift result right away,
                 * that decode penalty is probably less than the latency
                 * we'd see from SHRD. */
                if (host_src1 != host_dest) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, host_src1);
                }
                append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_dest);
                append_imm8(&code, insn->bitfield.start);
                host_shifted = host_dest;
            } else {
                host_shifted = host_src1;
            }

            const int operand_size = is64 ? 64 : 32;
            if (insn->bitfield.start + insn->bitfield.count < operand_size) {
                if (insn->bitfield.count < 8) {
                    if (host_shifted != host_dest) {
                        append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                              host_dest, host_shifted);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                          X86OP_IMM_AND, host_dest);
                    append_imm8(&code, (1U << insn->bitfield.count) - 1);
                } else if (insn->bitfield.count == 8) {
                    /* Registers SP-DI require an empty REX prefix to
                     * access the low byte as a byte register, but be
                     * careful not to double REX if a prefix will already
                     * be added. */
                    if (host_shifted >= X86_SP && host_shifted <= X86_DI
                     && host_dest <= X86_DI) {
                        append_opcode(&code, X86OP_REX);
                    }
                    append_insn_ModRM_reg(&code, is64, X86OP_MOVZX_Gv_Eb,
                                          host_dest, host_shifted);
                } else if (insn->bitfield.count == 16) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOVZX_Gv_Ew,
                                          host_dest, host_shifted);
                } else if (insn->bitfield.count < 32) {
                    if (host_shifted != host_dest) {
                        append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                              host_dest, host_shifted);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                          X86OP_IMM_AND, host_dest);
                    append_imm32(&code, (1U << insn->bitfield.count) - 1);
                } else if (insn->bitfield.count == 32) {
                    append_insn_ModRM_reg(&code, false, X86OP_MOV_Gv_Ev,
                                          host_dest, host_shifted);
                } else {
                    X86Register host_andsrc;
                    if (host_shifted != host_dest) {
                        host_andsrc = host_shifted;
                        append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_dest);
                    } else {
                        ASSERT(ctx->regs[dest].temp_allocated);
                        const X86Register host_temp = ctx->regs[dest].host_temp;
                        host_andsrc = host_temp;
                        append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_temp);
                    }
                    append_imm32(&code, -1);
                    append_imm32(&code, (1U << (insn->bitfield.count-32)) - 1);
                    append_insn_ModRM_reg(&code, is64, X86OP_AND_Gv_Ev,
                                          host_dest, host_andsrc);
                }
            } else if (host_dest != host_shifted) {
                /* This implies that the instruction is "extracting" the
                 * entire register contents. */
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_shifted);
            }
            break;
          }  // case RTLOP_BFEXT

          case RTLOP_BFINS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            ASSERT(host_dest != host_src2);
            const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
            const int operand_size = is64 ? 64 : 32;

            if (UNLIKELY(insn->bitfield.count == operand_size)) {
                /* Handle this case specially not so much for optimization
                 * purposes (since it should normally be optimized to a
                 * simple move at the RTL level) but because handling it
                 * correctly in the normal path takes extra effort. */
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src2);
                break;
            }

            /* Copy the first source into the destination, masking off the
             * bits to be overwritten. */
            if (is64 && insn->bitfield.start + insn->bitfield.count > 31) {
                const uint64_t src2_mask =
                    (UINT64_C(1) << insn->bitfield.count) - 1;
                const uint64_t src1_mask = ~(src2_mask << insn->bitfield.start);
                if (host_dest == host_src1) {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    const X86Register host_temp = ctx->regs[dest].host_temp;
                    ASSERT(host_temp != host_src2);
                    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_temp);
                    append_imm32(&code, (uint32_t)src1_mask);
                    append_imm32(&code, (uint32_t)(src1_mask >> 32));
                    append_insn_ModRM_reg(&code, true, X86OP_AND_Gv_Ev,
                                          host_dest, host_temp);
                } else {
                    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_dest);
                    append_imm32(&code, (uint32_t)src1_mask);
                    append_imm32(&code, (uint32_t)(src1_mask >> 32));
                    append_insn_ModRM_reg(&code, true, X86OP_AND_Gv_Ev,
                                          host_dest, host_src1);
                }
            } else {
                const uint32_t src2_mask = (1U << insn->bitfield.count) - 1;
                const uint32_t src1_mask = ~(src2_mask << insn->bitfield.start);
                if (src1_mask == 0x000000FF) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOVZX_Gv_Eb,
                                          host_dest, host_src1);
                } else if (src1_mask == 0x0000FFFF) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOVZX_Gv_Ew,
                                          host_dest, host_src1);
                } else {
                    if (host_dest != host_src1) {
                        append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                              host_dest, host_src1);
                    }
                    if (src1_mask >= 0xFFFFFF80) {
                        append_insn_ModRM_reg(&code, is64, X86OP_IMM_Ev_Ib,
                                              X86OP_IMM_AND, host_dest);
                        append_imm8(&code, (uint8_t)src1_mask);
                    } else {
                        append_insn_ModRM_reg(&code, is64, X86OP_IMM_Ev_Iz,
                                              X86OP_IMM_AND, host_dest);
                        append_imm32(&code, src1_mask);
                    }
                }
            }

            /* Copy the bits to be inserted to the temporary register,
             * shifting them to the appropriate place.  But reuse src2 as
             * the temporary register if it dies on this instruction. */
            X86Register host_newbits;
            if (unit->regs[src2].death == insn_index) {
                host_newbits = host_src2;
            } else {
                ASSERT(ctx->regs[dest].temp_allocated);
                const X86Register host_temp = ctx->regs[dest].host_temp;
                ASSERT(host_temp != host_src2);
                host_newbits = host_temp;
            }
            if (insn->bitfield.count > 32) {
                /* We can't use a 64-bit immediate value with AND, so
                 * shift the value left and (if necessary) right again. */
                ASSERT(is64);
                if (host_newbits != host_src2) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          host_newbits, host_src2);
                }
                append_insn_ModRM_reg(&code, true, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHL, host_newbits);
                append_imm8(&code, 64 - insn->bitfield.count);
                const int shr_count =
                    64 - (insn->bitfield.start + insn->bitfield.count);
                if (shr_count > 0) {
                    append_insn_ModRM_reg(&code, true, X86OP_SHIFT_Ev_Ib,
                                          X86OP_SHIFT_SHR, host_newbits);
                    append_imm8(&code, shr_count);
                }
            } else {
                if (insn->bitfield.start + insn->bitfield.count == operand_size) {
                    if (host_newbits != host_src2) {
                        append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                              host_newbits, host_src2);
                    }
                } else if (insn->bitfield.count < 8) {
                    if (host_newbits != host_src2) {
                        append_insn_ModRM_reg(&code, false, X86OP_MOV_Gv_Ev,
                                              host_newbits, host_src2);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                          X86OP_IMM_AND, host_newbits);
                    append_imm8(&code, (1U << insn->bitfield.count) - 1);
                } else if (insn->bitfield.count == 8) {
                    /* Registers SP-DI require an empty REX prefix to
                     * access the low byte as a byte register, but be
                     * careful not to double REX if a prefix will already
                     * be added. */
                    if (host_src2 >= X86_SP && host_src2 <= X86_DI
                     && host_newbits <= X86_DI) {
                        append_opcode(&code, X86OP_REX);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Eb,
                                          host_newbits, host_src2);
                } else if (insn->bitfield.count == 16) {
                    append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Ew,
                                          host_newbits, host_src2);
                } else if (insn->bitfield.count == 32) {
                    append_insn_ModRM_reg(&code, false, X86OP_MOV_Gv_Ev,
                                          host_newbits, host_src2);
                } else {
                    if (host_newbits != host_src2) {
                        append_insn_ModRM_reg(&code, false, X86OP_MOV_Gv_Ev,
                                              host_newbits, host_src2);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                          X86OP_IMM_AND, host_newbits);
                    append_imm32(&code, (1U << insn->bitfield.count) - 1);
                }
                if (insn->bitfield.start > 0) {
                    append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_Ib,
                                          X86OP_SHIFT_SHL, host_newbits);
                    append_imm8(&code, insn->bitfield.start);
                }
            }

            /* OR the new bits into the destination. */
            append_insn_ModRM_reg(&code, is64, X86OP_OR_Gv_Ev,
                                  host_dest, host_newbits);

            break;
          }  // case RTLOP_BFINS

          case RTLOP_LOAD_IMM: {
            const uint64_t imm = insn->src_imm;
            const X86Register host_dest = ctx->regs[dest].host_reg;
            switch (unit->regs[dest].type) {
              case RTLTYPE_INT32:
              case RTLTYPE_ADDRESS:
                if (imm == 0) {
                    append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                          host_dest, host_dest);
                    // FIXME: need to go back and fill these in all over
                    ctx->cc_reg = dest;
                } else if (imm <= UINT64_C(0xFFFFFFFF)) {
                    append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_dest);
                    append_imm32(&code, (uint32_t)imm);
                } else if (imm >= UINT64_C(0xFFFFFFFF80000000)) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Ev_Iz,
                                          0, host_dest);
                    append_imm32(&code, (uint32_t)imm);
                } else {
                    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_dest);
                    append_imm32(&code, (uint32_t)imm);
                    append_imm32(&code, (uint32_t)(imm >> 32));
                }
                break;
              default:
                // FIXME: handle FP registers when we support those
                ASSERT(!"Invalid data type in LOAD_IMM");
            }
            break;
          }  // case RTLOP_LOAD_IMM

          case RTLOP_LOAD_ARG: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const int host_src_i =
                host_x86_int_arg_register(ctx, insn->arg_index);
            ASSERT(host_src_i >= 0);  // Ensured by register allocation.
            const X86Register host_src = (X86Register)host_src_i;
            if (host_dest != host_src) {
                const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src);
            }
            break;
          }  // case RTLOP_LOAD_ARG

          case RTLOP_LABEL:
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(ctx->label_offsets[insn->label] == -1);
            ctx->label_offsets[insn->label] = code.len;
            break;

          case RTLOP_GOTO:
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(block_info->unresolved_branch_offset == -1);
            if (ctx->label_offsets[insn->label] >= 0) {
                const long offset = ctx->label_offsets[insn->label] - code.len;
                ASSERT(offset < 0);
                /* Jump distances count from the end of the instruction,
                 * so we have to take that into account here -- a 1-byte
                 * displacement will be a 2-byte instruction, for example. */
                if (offset - 2 >= -128) {
                    append_opcode(&code, X86OP_JMP_Jb);
                    append_imm8(&code, offset - 2);
                } else {
                    append_opcode(&code, X86OP_JMP_Jz);
                    append_imm32(&code, offset - 5);
                }
            } else {
                block_info->unresolved_branch_offset = code.len;
                block_info->unresolved_branch_target = insn->label;
                append_opcode(&code, X86OP_JMP_Jz);
                append_imm32(&code, 0);
            }
            break;

          case RTLOP_GOTO_IF_Z:
          case RTLOP_GOTO_IF_NZ: {
            const X86Opcode short_jump =
                (insn->opcode == RTLOP_GOTO_IF_Z ? X86OP_JZ_Jb : X86OP_JNZ_Jb);
            const X86Opcode long_jump =
                (insn->opcode == RTLOP_GOTO_IF_Z ? X86OP_JZ_Jz : X86OP_JNZ_Jz);

            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(block_info->unresolved_branch_offset == -1);
            append_test_reg(ctx, &code, src1);
            if (ctx->label_offsets[insn->label] >= 0) {
                const long offset = ctx->label_offsets[insn->label] - code.len;
                ASSERT(offset < 0);
                if (offset - 2 >= -128) {
                    append_opcode(&code, short_jump);
                    append_imm8(&code, offset - 2);
                } else {
                    append_opcode(&code, long_jump);
                    /* Long conditional jumps have 2-byte opcodes. */
                    append_imm32(&code, offset - 6);
                }
            } else {
                block_info->unresolved_branch_offset = code.len;
                block_info->unresolved_branch_target = insn->label;
                append_opcode(&code, long_jump);
                append_imm32(&code, 0);
            }
            break;
          }  // case RTLOP_GOTO_IF_Z, RTLOP_GOTO_IF_NZ

          case RTLOP_RETURN:
            ASSERT(block_info->unresolved_branch_offset == -1);
            if (src1) {
                const X86Register host_src1 = ctx->regs[src1].host_reg;
                if (host_src1 != X86_AX) {
                    const RTLRegister * const src1_reg = &unit->regs[src1];
                    uint8_t rex;
                    if (src1_reg->type == RTLTYPE_INT32) {
                        rex = 0;
                    } else {
                        ASSERT(src1_reg->type == RTLTYPE_ADDRESS);
                        rex = X86_REX_W;
                    }
                    if (src1 & 8) {
                        rex |= X86_REX_B;
                    }
                    if (rex) {
                        append_rex_opcode(&code, rex, X86OP_MOV_Gv_Ev);
                    } else {
                        append_opcode(&code, X86OP_MOV_Gv_Ev);
                    }
                    append_ModRM(&code, X86MOD_REG, X86_AX, host_src1 & 7);
                }
            }
            block_info->unresolved_branch_offset = code.len;
            /* We use label 0 (normally invalid) to indicate a jump to the
             * function epilogue. */
            block_info->unresolved_branch_target = 0;
            append_opcode(&code, X86OP_JMP_Jz);
            append_imm32(&code, 0);
            break;

          case RTLOP_ILLEGAL:
            append_opcode(&code, X86OP_UD2);
            break;

          default: break; //FIXME: other insns not yet implemented

        }

        ASSERT(code.len - initial_len <= MAX_INSN_LEN);
    }

    handle->code_len = code.len;
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
    const uint32_t regs_to_save = ctx->regs_touched & ctx->callee_saved_regs;
    const bool is_windows_seh =
        (ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);

    /* Figure out how much stack space we use in total. */
    int total_stack_use;
    const int push_size = 8 * popcnt32(regs_to_save & 0xFFFF);
    total_stack_use = push_size;
    /* We have to realign the stack at this specific point; if we're saving
     * XMM registers then those need to be aligned, and in any case we
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
            UWOP_ALLOC_LARGE = 1,
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

        int sp_offset = ctx->frame_size;
        for (int reg = 16; reg < 32; reg++) {
            if (regs_to_save & (1 << reg)) {
                unwind_pos -= 4;
                ASSERT(unwind_pos >= 4);
                unwind_info[unwind_pos+0] = prologue_pos;
                unwind_info[unwind_pos+1] = UWOP_SAVE_XMM128 | reg<<4;
                unwind_info[unwind_pos+2] = (uint8_t)(sp_offset >> 4);
                unwind_info[unwind_pos+3] = (uint8_t)(sp_offset >> 12);
                if (reg & 8) {
                    prologue_pos += 5;  // REX + MOVAPS + ModR/M + SIB
                } else {
                    prologue_pos += 4;  // MOVAPS + ModR/M + SIB
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

        const int alignment = ctx->handle->code_alignment;
        ASSERT(alignment >= 8);
        int code_offset = 8 + size;
        if (code_offset % alignment != 0) {
            code_offset += alignment - (code_offset % alignment);
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

    /* In the worst case (Windows ABI with all registers saved and a frame
     * size of >=128 bytes), the prologue will require:
     *    1 * 4 low GPR saves
     *    2 * 4 high GPR saves
     *    7 * 1 stack adjustment
     *    8 * 2 low XMM saves
     *    9 * 8 high XMM saves
     * for a total of 107 bytes. */
    if (UNLIKELY(!binrec_ensure_code_space(ctx->handle, 107))) {
        return false;
    }

    CodeBuffer code = {.buffer = handle->code_buffer,
                       .buffer_size = handle->code_buffer_size,
                       .len = handle->code_len};

    for (int reg = 0; reg < 16; reg++) {
        if (regs_to_save & (1 << reg)) {
            append_insn_R(&code, false, X86OP_PUSH_rAX, reg);
        }
    }

    if (stack_alloc >= 128) {
        append_opcode(&code, X86OP_REX_W);
        append_opcode(&code, X86OP_IMM_Ev_Iz);
        append_ModRM(&code, X86MOD_REG, X86OP_IMM_SUB, X86_SP);
        append_imm32(&code, stack_alloc);
    } else if (stack_alloc > 0) {
        append_opcode(&code, X86OP_REX_W);
        append_opcode(&code, X86OP_IMM_Ev_Ib);
        append_ModRM(&code, X86MOD_REG, X86OP_IMM_SUB, X86_SP);
        append_imm8(&code, stack_alloc);
    }

    int sp_offset = ctx->frame_size;
    for (int reg = 16; reg < 32; reg++) {
        if (regs_to_save & (1 << reg)) {
            if (reg & 8) {
                append_rex_opcode(&code, X86OP_REX_R, X86OP_MOVAPS_W_V);
            } else {
                append_opcode(&code, X86OP_MOVAPS_W_V);
            }
            if (sp_offset >= 128) {
                append_ModRM_SIB(&code, X86MOD_DISP32, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm32(&code, sp_offset);
            } else if (sp_offset > 0) {
                append_ModRM_SIB(&code, X86MOD_DISP8, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm8(&code, sp_offset);
            } else {
                append_ModRM_SIB(&code, X86MOD_DISP0, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
            }
            sp_offset += 16;
        }
    }

    handle->code_len = code.len;

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
    const uint32_t regs_saved = ctx->regs_touched & ctx->callee_saved_regs;
    const int stack_alloc = ctx->stack_alloc;

    ctx->label_offsets[0] = handle->code_len;

    /* The maximum size of the epilogue is the same as the maximum size of
     * the prologue, plus 1 for the RET instruction. */
    if (UNLIKELY(!binrec_ensure_code_space(handle, 108))) {
        return false;
    }

    CodeBuffer code = {.buffer = handle->code_buffer,
                       .buffer_size = handle->code_buffer_size,
                       .len = handle->code_len};

    int sp_offset = ctx->frame_size + 16 * popcnt32(regs_saved >> 16);
    for (int reg = 31; reg >= 16; reg--) {
        if (regs_saved & (1 << reg)) {
            sp_offset -= 16;
            if (reg & 8) {
                append_rex_opcode(&code, X86OP_REX_R, X86OP_MOVAPS_V_W);
            } else {
                append_opcode(&code, X86OP_MOVAPS_V_W);
            }
            if (sp_offset >= 128) {
                append_ModRM_SIB(&code, X86MOD_DISP32, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm32(&code, sp_offset);
            } else if (sp_offset > 0) {
                append_ModRM_SIB(&code, X86MOD_DISP8, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
                append_imm8(&code, sp_offset);
            } else {
                append_ModRM_SIB(&code, X86MOD_DISP0, reg & 7,
                                 0, X86SIB_NOINDEX, X86_SP);
            }
        }
    }

    if (stack_alloc >= 128) {
        append_opcode(&code, X86OP_REX_W);
        append_opcode(&code, X86OP_IMM_Ev_Iz);
        append_ModRM(&code, X86MOD_REG, X86OP_IMM_ADD, X86_SP);
        append_imm32(&code, stack_alloc);
    } else if (stack_alloc > 0) {
        append_opcode(&code, X86OP_REX_W);
        append_opcode(&code, X86OP_IMM_Ev_Ib);
        append_ModRM(&code, X86MOD_REG, X86OP_IMM_ADD, X86_SP);
        append_imm8(&code, stack_alloc);
    }

    for (int reg = 15; reg >= 0; reg--) {
        if (regs_saved & (1 << reg)) {
            append_insn_R(&code, false, X86OP_POP_rAX, reg);
        }
    }

    append_opcode(&code, X86OP_RET);

    handle->code_len = code.len;

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
 */
static void resolve_branches(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);

    for (int i = 0; i >= 0; i = ctx->unit->blocks[i].next_block) {
        HostX86BlockInfo *block_info = &ctx->blocks[i];
        const long branch_offset = block_info->unresolved_branch_offset;
        if (branch_offset != -1) {
            const uint32_t label = block_info->unresolved_branch_target;
            ASSERT(label < ctx->unit->next_label);
            ASSERT(ctx->label_offsets[label] != -1);
            long offset = ctx->label_offsets[label] - branch_offset;
            ASSERT(offset > 0);  // Or else it would have been resolved.
            ASSERT(offset < INT64_C(0x80000000));  // Sanity check.
            uint8_t *ptr = &ctx->handle->code_buffer[branch_offset];
            if (*ptr == 0x0F) {
                ptr += 2;
                offset -= 6;
            } else {
                ptr += 1;
                offset -= 5;
            }
            ptr[0] = (uint8_t)(offset >>  0);
            ptr[1] = (uint8_t)(offset >>  8);
            ptr[2] = (uint8_t)(offset >> 16);
            ptr[3] = (uint8_t)(offset >> 24);
        }
    }
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

    resolve_branches(ctx);

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
    for (int i = 0; i < unit->num_blocks; i++) {
        ctx->blocks[i].unresolved_branch_offset = -1;
    }
    memset(ctx->regs, 0, sizeof(*ctx->regs) * unit->next_reg);
    memset(ctx->label_offsets, -1,
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
