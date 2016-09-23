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

/*-----------------------------------------------------------------------*/

/**
 * is_spilled:  Helper function to return whether a register is currently
 * spilled.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     reg: RTL register number.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 * [Return value]
 *     True if the register is spilled at insn_index, false if not.
 */
static inline PURE_FUNCTION bool is_spilled(const HostX86Context *ctx, int reg,
                                            int insn_index)
{
    const HostX86RegInfo *reg_info = &ctx->regs[reg];
    return reg_info->spilled && reg_info->spill_insn <= insn_index;
}

/*-----------------------------------------------------------------------*/

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
        ASSERT(opcode>>24 == 0x66 || opcode>>24 == 0xF2 || opcode>>24 == 0xF3);
        *ptr++ = opcode >> 24;
        *ptr++ = rex;
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
 * append_insn:  Append an instruction which takes no operands.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True for 64-bit operands, false for 32-bit operands.
 *     opcode: Instruction opcode.
 */
static APPEND_INLINE void append_insn(
    CodeBuffer *code, bool is64, X86Opcode opcode)
{
    if (is64) {
        append_rex_opcode(code, X86_REX_W, opcode);
    } else {
        append_opcode(code, opcode);
    }
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

/*-----------------------------------------------------------------------*/

/**
 * append_insn_ModRM_mem:  Append an instruction which takes a ModR/M byte,
 * encoding a base-plus-offset memory EA in the instruction.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True for 64-bit operands, false for 32-bit operands.
 *     opcode: Instruction opcode.
 *     reg: Register index (or sub-opcode) for ModR/M reg field.
 *     base: Base register index for memory address.
 *     offset: Constant offset for memory address.
 */
static APPEND_INLINE void append_insn_ModRM_mem(
    CodeBuffer *code, bool is64, X86Opcode opcode, X86Register reg,
    X86Register base, int32_t offset)
{
    uint8_t rex = is64 ? X86_REX_W : 0;
    if (reg & 8) {
        rex |= X86_REX_R;
    }
    if (base & 8) {
        rex |= X86_REX_B;
    }
    if (rex) {
        append_rex_opcode(code, rex, opcode);
    } else {
        append_opcode(code, opcode);
    }

    X86Mod mod;
    if (offset == 0) {
        /* The x86 ISA doesn't allow dereferencing rBP with no offset;
         * a ModR/M byte with mod=0, r/m=5 (BP) is interpreted as an
         * absolute address (RIP-relative in x86-64 mode).  Instead,
         * we have to encode it as an 8-bit displacement of zero.
         * This also applies to R13 in x86-64 mode, since the hardware
         * does not check REX.B before invoking special handling for
         * that ModR/M combination. */
        if (base == X86_BP || base == X86_R13) {
            mod = X86MOD_DISP8;
        } else {
            mod = X86MOD_DISP0;
        }
    } else if ((uint32_t)offset + 128 < 256) {  // [-128,+127]
        mod = X86MOD_DISP8;
    } else {
        mod = X86MOD_DISP32;
    }
    if (base == X86_SP || base == X86_R12) {
        /* SP (4) in the r/m field is used to indicate the presence of a
         * SIB byte, so we have to encode SP references using SIB.  This
         * also applies to R12, for the same reason as R13 with respect
         * to BP (see above). */
        append_ModRM_SIB(code, mod, reg & 7, 0, X86SIB_NOINDEX, X86_SP);
    } else {
        append_ModRM(code, mod, reg & 7, base & 7);
    }
    if (mod == X86MOD_DISP8) {
        append_imm8(code, (uint8_t)offset);
    } else if (mod == X86MOD_DISP32) {
        append_imm32(code, (uint32_t)offset);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_insn_ModRM_ctx:  Append an instruction which takes a ModR/M byte,
 * encoding an EA appropriate to the given source RTL register.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True for 64-bit operands, false for 32-bit operands.
 *     opcode: Instruction opcode.
 *     reg1: Register index (or sub-opcode) for ModR/M reg field.
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     rtl_reg2: RTL register index from which to set ModR/M r/m field.
 */
static APPEND_INLINE void append_insn_ModRM_ctx(
    CodeBuffer *code, bool is64, X86Opcode opcode, X86Register reg1,
    HostX86Context *ctx, int insn_index, int rtl_reg2)
{
    if (is_spilled(ctx, rtl_reg2, insn_index)) {
        append_insn_ModRM_mem(code, is64, opcode, reg1,
                              X86_SP, ctx->regs[rtl_reg2].spill_offset);
    } else {
        append_insn_ModRM_reg(code, is64, opcode, reg1,
                              ctx->regs[rtl_reg2].host_reg);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_move:  Append an instruction to copy (MOV) one register to another.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     type: Register type (RTLTYPE_*).
 *     host_dest: Destination host register.
 *     host_src: Source host register.
 */
static APPEND_INLINE void append_move(
    CodeBuffer *code, RTLDataType type, X86Register host_dest,
    X86Register host_src)
{
    switch (type) {
      case RTLTYPE_INT32:
        append_insn_ModRM_reg(code, false, X86OP_MOV_Gv_Ev,
                              host_dest, host_src);
        break;
      case RTLTYPE_ADDRESS:
        append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                              host_dest, host_src);
        break;
      case RTLTYPE_FLOAT:
      case RTLTYPE_DOUBLE:
      case RTLTYPE_V2_DOUBLE:
        /* The Intel optimization guidelines state: (1) avoid mixed use of
         * integer/FP operations on the same register (thus MOVAPS instead
         * of MOVDQA); (2) use PS instead of PD if both operations are
         * bitwise-equivalent (thus MOVAPS even for double-precision types). */
        append_insn_ModRM_reg(code, false, X86OP_MOVAPS_V_W,
                              host_dest, host_src);
        break;
        break;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_move_gpr:  Append an instruction to copy (MOV) one integer
 * register to another.
 *
 * Specialization of append_move() for GPRs.
 */
static APPEND_INLINE void append_move_gpr(
    CodeBuffer *code, RTLDataType type, X86Register host_dest,
    X86Register host_src)
{
    ASSERT(rtl_type_is_int(type));
    append_insn_ModRM_reg(code, type == RTLTYPE_ADDRESS, X86OP_MOV_Gv_Ev,
                          host_dest, host_src);
}

/*-----------------------------------------------------------------------*/

/**
 * append_load:  Append an instruction to load a register from a memory
 * location.  The memory address is assumed to be properly aligned.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     type: Register type (RTLTYPE_*).
 *     host_dest: Destination host register.
 *     host_base: Host register for memory address base.
 *     offset: Access offset from base register.
 */
static APPEND_INLINE void append_load(
    CodeBuffer *code, RTLDataType type, X86Register host_dest,
    X86Register host_base, int32_t offset)
{
    switch (type) {
      case RTLTYPE_INT32:
        append_insn_ModRM_mem(code, false, X86OP_MOV_Gv_Ev,
                              host_dest, host_base, offset);
        break;
      case RTLTYPE_ADDRESS:
        append_insn_ModRM_mem(code, true, X86OP_MOV_Gv_Ev,
                              host_dest, host_base, offset);
        break;
      case RTLTYPE_FLOAT:
        append_insn_ModRM_mem(code, false, X86OP_MOVSS_V_W,
                              host_dest, host_base, offset);
        break;
      case RTLTYPE_DOUBLE:
        append_insn_ModRM_mem(code, false, X86OP_MOVSD_V_W,
                              host_dest, host_base, offset);
        break;
      case RTLTYPE_V2_DOUBLE:
        /* MOVAPS instead of MOVAPD for optimization purposes (see note in
         * append_move()). */
        append_insn_ModRM_mem(code, false, X86OP_MOVAPS_V_W,
                              host_dest, host_base, offset);
        break;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_load_gpr:  Append an instruction to load an integer register
 * from a memory location.  The memory address is assumed to be properly
 * aligned.
 *
 * Specialization of append_load() for GPRs.
 */
static APPEND_INLINE void append_load_gpr(
    CodeBuffer *code, RTLDataType type, X86Register host_dest,
    X86Register host_base, int32_t offset)
{
    ASSERT(rtl_type_is_int(type));
    append_insn_ModRM_mem(code, type == RTLTYPE_ADDRESS, X86OP_MOV_Gv_Ev,
                          host_dest, host_base, offset);
}

/*-----------------------------------------------------------------------*/

/**
 * append_store:  Append an instruction to store a register to a memory
 * location.  The memory address is assumed to be properly aligned.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     type: Register type (RTLTYPE_*).
 *     host_src: Destination host register.
 *     host_base: Host register for memory address base.
 *     offset: Access offset from base register.
 */
static APPEND_INLINE void append_store(
    CodeBuffer *code, RTLDataType type, X86Register host_src,
    X86Register host_base, int32_t offset)
{
    switch (type) {
      case RTLTYPE_INT32:
        append_insn_ModRM_mem(code, false, X86OP_MOV_Ev_Gv,
                              host_src, host_base, offset);
        break;
      case RTLTYPE_ADDRESS:
        append_insn_ModRM_mem(code, true, X86OP_MOV_Ev_Gv,
                              host_src, host_base, offset);
        break;
      case RTLTYPE_FLOAT:
        append_insn_ModRM_mem(code, false, X86OP_MOVSS_W_V,
                              host_src, host_base, offset);
        break;
      case RTLTYPE_DOUBLE:
        append_insn_ModRM_mem(code, false, X86OP_MOVSD_W_V,
                              host_src, host_base, offset);
        break;
      case RTLTYPE_V2_DOUBLE:
        /* MOVAPS instead of MOVAPD for optimization purposes (see note in
         * append_move()). */
        append_insn_ModRM_mem(code, false, X86OP_MOVAPS_W_V,
                              host_src, host_base, offset);
        break;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_load_alias:  Append an instruction to load the given alias from
 * its storage location.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     ctx: Translation context.
 *     alias: Alias register to load.
 *     host_dest: Host register into which to load alias register value.
 */
static APPEND_INLINE void append_load_alias(
    CodeBuffer *code, const HostX86Context *ctx, const RTLAlias *alias,
    X86Register host_dest)
{
    const X86Register host_base =
        alias->base ? ctx->regs[alias->base].host_reg : X86_SP;
    append_load(code, alias->type, host_dest, host_base, alias->offset);
}

/*-----------------------------------------------------------------------*/

/**
 * append_store_alias:  Append an instruction to store the given alias to
 * its storage location.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     ctx: Translation context.
 *     alias: Alias to store.
 *     host_src: Host register containing data to store.
 */
static APPEND_INLINE void append_store_alias(
    CodeBuffer *code, const HostX86Context *ctx, const RTLAlias *alias,
    X86Register host_src)
{
    const X86Register host_base =
        alias->base ? ctx->regs[alias->base].host_reg : X86_SP;
    append_store(code, alias->type, host_src, host_base, alias->offset);
}

/*-----------------------------------------------------------------------*/

/**
 * append_move_or_load:  If the given source register has been spilled,
 * load it into the given destination register from its spill location;
 * otherwise, move it from its current register if it is not already in
 * the destination register.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     ctx: Translation context.
 *     unit: RTLUnit being translated.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     host_dest: Destination host register.
 *     src: Source RTL register.
 */
static APPEND_INLINE void append_move_or_load(
    CodeBuffer *code, const HostX86Context *ctx, const RTLUnit *unit,
    int insn_index, int host_dest, int src)
{
    if (is_spilled(ctx, src, insn_index)) {
        append_load(code, unit->regs[src].type, host_dest,
                    X86_SP, ctx->regs[src].spill_offset);
    } else if (ctx->regs[src].host_reg != host_dest) {
        append_move(code, unit->regs[src].type, host_dest,
                    ctx->regs[src].host_reg);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_move_or_load_gpr:  If the given source integer register has been
 * spilled, load it into the given destination register from its spill
 * location; otherwise, move it from its current register if it is not
 * already in the destination register.
 *
 * Specialization of append_load() for GPRs.
 */
static APPEND_INLINE void append_move_or_load_gpr(
    CodeBuffer *code, const HostX86Context *ctx, const RTLUnit *unit,
    int insn_index, int host_dest, int src)
{
    if (is_spilled(ctx, src, insn_index)) {
        append_load_gpr(code, unit->regs[src].type, host_dest,
                        X86_SP, ctx->regs[src].spill_offset);
    } else if (ctx->regs[src].host_reg != host_dest) {
        append_move_gpr(code, unit->regs[src].type, host_dest,
                        ctx->regs[src].host_reg);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_test_reg:  Append an instruction to test the value of the given
 * integer RTL register for zeroness.
 */
static APPEND_INLINE void append_test_reg(
    HostX86Context *ctx, const RTLUnit *unit, int insn_index,
    CodeBuffer *code, int reg)
{
    ASSERT(rtl_register_is_int(&unit->regs[reg]));

    if ((ctx->handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES)
     && ctx->cc_reg == reg) {
        return;  // Condition codes are already set appropriately.
    }

    if (is_spilled(ctx, reg, insn_index)) {
        const bool is64 = (unit->regs[reg].type == RTLTYPE_ADDRESS);
        append_insn_ModRM_mem(code, is64, X86OP_IMM_Ev_Ib, X86OP_IMM_CMP,
                              X86_SP, ctx->regs[reg].spill_offset);
        append_imm8(code, 0);
    } else {
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
    }

    ctx->cc_reg = reg;
}

/*-----------------------------------------------------------------------*/

/**
 * append_jump_raw:  Append a JMP or Jcc instruction with the given
 * displacement.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     short_opcode: Opcode for a short (8-bit) displacement.
 *     long_opcode: Opcode for a long (32-bit) displacement.
 *     disp: Displacement to encode.
 */
static APPEND_INLINE void append_jump_raw(
    CodeBuffer *code, X86Opcode short_opcode, X86Opcode long_opcode,
    int32_t disp)
{
    ASSERT((uint32_t)short_opcode <= 0xFF);
    if (((uint32_t)disp + 128) < 256) {  // i.e., disp is in [-128,+127]
        append_opcode(code, short_opcode);
        append_imm8(code, (uint8_t)disp);
    } else {
        append_opcode(code, long_opcode);
        append_imm32(code, disp);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_jump:  Append a JMP or Jcc instruction targeting the given code
 * location.
 *
 * If target is negative, a long jump with displacement 0 is appended and
 * the address of the displacement is saved as the current block's
 * unresolved branch.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     block_info: HostX86BlockInfo for the current basic block.
 *     short_opcode: Opcode for a short (8-bit) displacement.
 *     long_opcode: Opcode for a long (32-bit) displacement.
 *     label: Target label.
 *     target: Byte position of the target instruction, or -1 if the target
 *         is unknown.
 */
static APPEND_INLINE void append_jump(
    CodeBuffer *code, HostX86BlockInfo *block_info,
    X86Opcode short_opcode, X86Opcode long_opcode, int label, long target)
{
    if (target >= 0) {
        const long offset = target - code->len;
        ASSERT(offset >= INT64_C(-0x80000000) && offset <= INT64_C(0x7FFFFFFF));
        /* Jump displacements count from the end of the instruction, so we
         * have to take that into account here -- a 1-byte displacement
         * will be a 2-byte instruction, for example. */
        ASSERT((uint32_t)short_opcode <= 0xFF);
        if (((unsigned long)(offset - 2) + 128) < 256) {
            append_opcode(code, short_opcode);
            append_imm8(code, (uint8_t)(offset - 2));
        } else {
            append_opcode(code, long_opcode);
            if ((uint32_t)long_opcode <= 0xFF) {
                append_imm32(code, (uint32_t)(offset - 5));
            } else {
                ASSERT((uint32_t)long_opcode <= 0xFFFF);
                append_imm32(code, (uint32_t)(offset - 6));
            }
        }
    } else {
        append_opcode(code, long_opcode);
        block_info->unresolved_branch_offset = code->len;
        append_imm32(code, 0);
        block_info->unresolved_branch_target = label;
    }
}

/*************************************************************************/
/**************************** Alias handling *****************************/
/*************************************************************************/

/* Maximum length of alias setup code generated by setup_aliases_for_block().
 * If the input code buffer has at least this much space available, it will
 * not be expanded.  Worst case is:
 *    - 15 GPR loads with REX prefixes = 105 bytes
 *    - 15 XMM exchanges with REX prefixes = 180 bytes
 *    - 1 XMM load with a REX prefix = 9 bytes
 * for a total of 294 bytes.  We round up to 320 to potentially help the
 * compiler optimize. */
#define SETUP_ALIAS_SIZE  320

/**
 * setup_aliases_for_block:  Set up registers expected to contain alias
 * values on entry to the given block.
 *
 * The code buffer passed to this function may be a temporary buffer
 *
 * [Parameters]
 *     code: Output code buffer (may be a temporary buffer).
 *     ctx: Translation context.
 *     block_index: Index of current basic block in ctx->unit->blocks[].
 *     target_index: Index of target basic block in ctx->unit->blocks[].
 * [Return value]
 *     True on success, false if out of memory.
 */
static bool setup_aliases_for_block(
    CodeBuffer *code, HostX86Context *ctx, int block_index, int target_index)
{
    RTLUnit * const unit = ctx->unit;
    const int num_aliases = unit->next_alias;
    const uint16_t *current_store = ctx->blocks[block_index].alias_store;
    const uint16_t *next_load = ctx->blocks[target_index].alias_load;

    if (UNLIKELY(code->buffer_size - code->len < SETUP_ALIAS_SIZE)) {
        ASSERT(code->buffer == ctx->handle->code_buffer);
        ctx->handle->code_len = code->len;
        if (UNLIKELY(!binrec_ensure_code_space(ctx->handle,
                                               SETUP_ALIAS_SIZE))) {
            return false;
        }
        code->buffer = ctx->handle->code_buffer;
        code->buffer_size = ctx->handle->code_buffer_size;
    }

    /* First construct a map of which registers get moved where and which
     * need to be loaded from storage.  We need a separate step for this in
     * case the target of one move would overwrite the source of another,
     * in which case we have to use a swap instead.  This can get
     * particularly tricky if several registers are shifted in a loop (see
     * tests/host-x86/general/alias-merge-swap-cycle-* for examples). */
    uint32_t move_targets = 0;  // Bit set = register is a move target
    uint32_t load_targets = 0;  // Bit set = register is a load target
    uint8_t move_map[32];  // X86Register source for each move target
    uint8_t src_map[32];   // Map from original to current (swapped) registers
                           //    (i.e., "where is this register's value now?")
    uint8_t value_map[32]; // Map from original reg values to current locations
                           //    (i.e., "what does this register now hold?")
    uint8_t src_count[32]; // # of times each host register is used as a source
    uint16_t load_map[32]; // RTL alias to load into each load target
    uint8_t dest_type[32]; // RTLDataType of each alias, indexed by move target
    memset(src_count, 0, sizeof(src_count));
    for (int i = 1; i < num_aliases; i++) {
        const int merge_reg = next_load[i];
        if (merge_reg && ctx->regs[merge_reg].merge_alias) {
            const X86Register host_dest = ctx->regs[merge_reg].host_merge;
            dest_type[host_dest] = unit->regs[merge_reg].type;
            value_map[host_dest] = host_dest;
            const int store_reg = current_store[i];
            if (store_reg) {
                const X86Register host_src = ctx->regs[store_reg].host_reg;
                move_targets |= 1u << host_dest;
                move_map[host_dest] = host_src;
                src_map[host_src] = host_src;
                value_map[host_src] = host_src;
                src_count[host_src]++;
            } else {
                load_targets |= 1u << host_dest;
                load_map[host_dest] = (uint16_t)i;
            }
        }
    }

    /* Now actually perform the moves, swapping registers as needed.
     * There's no equivalent of the XCHG instruction for XMM registers,
     * so we implement XMM swaps using the XOR method (a^=b, b^=a, a^=b). */
    while (move_targets) {
        const X86Register host_dest = ctz32(move_targets);
        move_targets ^= 1u << host_dest;
        const X86Register move_src = move_map[host_dest];
        const X86Register host_src = src_map[move_src];
        if (host_src != host_dest) {
            if (src_count[value_map[host_dest]] > 0) {
                /* The value in the register we're about to write is still
                 * needed.  Swap it with the source value for this alias,
                 * then update maps so we know where the values have gone. */
                if (host_dest >= X86_XMM0) {
                    ASSERT(host_src >= X86_XMM0);
                    append_insn_ModRM_reg(code, false, X86OP_XORPS, 
                                          host_dest, host_src);
                    append_insn_ModRM_reg(code, false, X86OP_XORPS, 
                                          host_src, host_dest);
                    append_insn_ModRM_reg(code, false, X86OP_XORPS, 
                                          host_dest, host_src);
                } else {
                    ASSERT(host_src < X86_XMM0);
                    append_insn_ModRM_reg(code, true, X86OP_XCHG_Ev_Gv,
                                          host_src, host_dest);
                }
                value_map[host_src] = value_map[host_dest];
                src_map[value_map[host_dest]] = host_src;
                value_map[host_dest] = move_src;
                src_map[move_src] = host_dest;
            } else {
                append_move(code, dest_type[host_dest], host_dest, host_src);
            }
        }
        src_count[move_src]--;
    }

    /* Finally, load values from storage which were not live. */
    while (load_targets) {
        const X86Register host_dest = ctz32(load_targets);
        load_targets ^= 1u << host_dest;
        append_load_alias(code, ctx, &unit->aliases[load_map[host_dest]],
                          host_dest);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * check_alias_conflicts:  Return whether any alias loads required for the
 * flow graph edge from block_index to target_index collide with any live
 * registers.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of current basic block in ctx->unit->blocks[].
 *     target_index: Index of target basic block in ctx->unit->blocks[].
 * [Return value]
 *     True if any alias loads would collide with a live register, false
 *     otherwise.
 */
static bool check_alias_conflicts(const HostX86Context *ctx, int block_index,
                                  int target_index)
{
    RTLUnit * const unit = ctx->unit;
    const int num_aliases = unit->next_alias;
    const uint32_t end_live = ctx->blocks[block_index].end_live;
    const uint16_t *current_store = ctx->blocks[block_index].alias_store;
    const uint16_t *next_load = ctx->blocks[target_index].alias_load;

    for (int i = 1; i < num_aliases; i++) {
        const int merge_reg = next_load[i];
        if (merge_reg && ctx->regs[merge_reg].merge_alias) {
            const X86Register host_src = ctx->regs[current_store[i]].host_reg;
            const X86Register host_dest = ctx->regs[merge_reg].host_merge;
            if (host_dest != host_src && (end_live & (1 << host_dest))) {
                return true;
            }
        }
    }

    return false;
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
 *     True on success, false if out of memory.
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

    block_info->unresolved_branch_offset = -1;
    bool fall_through = true;  // Does code fall through to the next block?

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        const RTLInsn * const insn = &unit->insns[insn_index];
        ASSERT(insn->opcode >= RTLOP__FIRST);
        ASSERT(insn->opcode <= RTLOP__LAST);
        const int dest = insn->dest;
        const int src1 = insn->src1;
        const int src2 = insn->src2;

        /* Verify (if ENABLE_ASSERT) that all generated code fits within
         * the space we reserve per instruction here.  Currently, the worst
         * possible case is BFINS with spill of dest, spilled src1 and src2,
         * all stack offsets >= 128, and a bit count greater than 32 (see
         * tests/host-x86/insn/bfins-spilled-max-output-len.c). */
        const int MAX_INSN_LEN = 45;
        if (UNLIKELY(code.len + MAX_INSN_LEN > code.buffer_size)) {
            handle->code_len = code.len;
            if (UNLIKELY(!binrec_ensure_code_space(handle, MAX_INSN_LEN))) {
                return false;
            }
            code.buffer = handle->code_buffer;
            code.buffer_size = handle->code_buffer_size;
        }

        /* This is not "const" because we rewrite it in GOTO_IF_* if we
         * add alias conflict resolution code, since that has its own
         * buffer size check. */
        long initial_len = code.len;

        /* Evict the current occupant of the destination register if needed. */
        if (dest) {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const int spill_index = ctx->reg_map[host_dest];
            if (spill_index) {
                const RTLRegister *spill_reg = &unit->regs[spill_index];
                if (spill_reg->death > insn_index) {
                    const HostX86RegInfo *spill_info = &ctx->regs[spill_index];
                    ASSERT(spill_info->spilled);
                    ASSERT(spill_info->spill_insn == insn_index);
                    append_store(&code, spill_reg->type, spill_info->host_reg,
                                 X86_SP, spill_info->spill_offset);
                }
            }
            ctx->reg_map[host_dest] = dest;
        }

        switch ((RTLOpcode)insn->opcode) {

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

          case RTLOP_SET_ALIAS: {
            if (src1 != block_info->alias_store[insn->alias]) {
                break;  // This is a dead store.
            }
            /* We need to store to memory if (1) this is a terminal block
             * or (2) at least one successor block doesn't both (a) have a
             * mergeable GET_ALIAS and (b) SET_ALIAS the same alias. */
            bool need_store = (block->exits[0] < 0);
            for (int i = 0; !need_store && i < lenof(block->exits); i++) {
                const int successor = block->exits[i];
                if (successor >= 0) {
                    const int reg =
                        ctx->blocks[successor].alias_load[insn->alias];
                    if (!reg
                     || !ctx->regs[reg].merge_alias
                     || !ctx->blocks[successor].alias_store[insn->alias]) {
                        need_store = true;
                    }
                }
            }
            if (need_store) {
                if (is_spilled(ctx, src1, insn_index)) {
                    const RTLRegister *src1_reg = &unit->regs[src1];
                    const X86Register temp_reg =
                        (rtl_register_is_int(src1_reg) ? X86_R15 : X86_XMM15);
                    append_load(&code, src1_reg->type, temp_reg,
                                X86_SP, ctx->regs[src1].spill_offset);
                    append_store_alias(&code, ctx, &unit->aliases[insn->alias],
                                       temp_reg);
                } else {
                    append_store_alias(&code, ctx, &unit->aliases[insn->alias],
                                       ctx->regs[src1].host_reg);
                }
            }
            break;
          }  // case RTLOP_SET_ALIAS

          case RTLOP_GET_ALIAS:
            /* Register allocation informs us whether we need to load
             * from memory. */
            if (!ctx->regs[dest].merge_alias) {
                append_load_alias(&code, ctx, &unit->aliases[insn->alias],
                                  ctx->regs[dest].host_reg);
            } else if (ctx->regs[dest].host_merge != ctx->regs[dest].host_reg) {
                /* The value is already loaded, but we need to move it to
                 * a different register. */
                append_move(&code, unit->regs[dest].type,
                            ctx->regs[dest].host_reg,
                            ctx->regs[dest].host_merge);
            }
            break;

          case RTLOP_MOVE:
            append_move_or_load(&code, ctx, unit, insn_index,
                                ctx->regs[dest].host_reg, src1);
            break;

          case RTLOP_SELECT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);

            /* Set the x86 condition flags based on the condition register. */
            append_test_reg(ctx, unit, insn_index, &code, insn->cond);

            /* Put one of the source values in the destination register, if
             * necessary.  Note that MOV does not alter flags. */
            bool dest_is_src1;
            if (host_dest == host_src1 && !is_spilled(ctx, src1, insn_index)) {
                dest_is_src1 = true;
            } else if (host_dest == host_src2
                       && !is_spilled(ctx, src2, insn_index)) {
                dest_is_src1 = false;
            } else {
                dest_is_src1 = true;
                append_move_or_load(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            }

            /* Conditionally move the other value into the register. */
            if (dest_is_src1) {
                append_insn_ModRM_ctx(&code, is64, X86OP_CMOVZ, host_dest,
                                      ctx, insn_index, src2);
            } else {
                append_insn_ModRM_ctx(&code, is64, X86OP_CMOVNZ, host_dest,
                                      ctx, insn_index, src1);
            }
            break;
          }  // case RTLOP_SELECT

          case RTLOP_SCAST:
          case RTLOP_ZCAST: {
            const RTLDataType type_dest = unit->regs[dest].type;
            const RTLDataType type_src1 = unit->regs[src1].type;
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_src1 = ctx->regs[src1].host_reg;
            if (is_spilled(ctx, src1, insn_index)) {
                append_load(&code, type_src1, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_src1 = host_dest;
            }
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
                        // FIXME: this should be unneeded since all 32-bit ops clear high word
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
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            if (is_spilled(ctx, src1, insn_index)) {
                append_load(&code, unit->regs[src1].type, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
            } else if (host_dest != host_src1) {
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
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_ADD ? X86OP_ADD_Gv_Ev :
                insn->opcode == RTLOP_SUB ? X86OP_SUB_Gv_Ev :
                insn->opcode == RTLOP_AND ? X86OP_AND_Gv_Ev :
                insn->opcode == RTLOP_OR ? X86OP_OR_Gv_Ev :
                             /* RTLOP_XOR */ X86OP_XOR_Gv_Ev);
            if (host_dest == host_src2 && !is_spilled(ctx, src2, insn_index)) {
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src1);
            } else {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src2);
            }
            break;
          }  // case RTLOP_{ADD,SUB,AND,OR,XOR}

          case RTLOP_MUL: {
            /* This case is identical to RTLOP_ADD (etc.), but it's
             * separated out to aid optimization, since the x86 IMUL
             * instruction is two bytes (0F AF) as opposed to the other
             * ALU instructions which are one byte. */
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = X86OP_IMUL_Gv_Ev;
            if (host_dest == host_src2 && !is_spilled(ctx, src2, insn_index)) {
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src1);
            } else {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src2);
            }
            break;
          }  // case RTLOP_MUL

          case RTLOP_MULHU:
          case RTLOP_MULHS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            ASSERT(host_dest != X86_AX);

            /* The destination register will always get rDX if it's free,
             * so if the destination is somewhere else, we need to save
             * away the current value of RDX.  We save all 64 bits of the
             * register so we don't have to worry about checking the type
             * of what's already there. */
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

            int multiplier;
            X86Register host_mult;
            if (host_src2 == X86_AX && !is_spilled(ctx, src2, insn_index)) {
                multiplier = src1;
                host_mult = host_src1;
            } else {
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
                /* Can't use append_move_or_load_gpr() here because of the
                 * possible rDX swap. */
                if (is_spilled(ctx, src1, insn_index)) {
                    append_load(&code, unit->regs[src1].type, X86_AX,
                                X86_SP, ctx->regs[src1].spill_offset);
                } else if (host_src1 != X86_AX) {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          X86_AX, multiplicand);
                }
                multiplier = src2;
                host_mult = host_src2;
            }

            const X86UnaryOpcode opcode = (
                insn->opcode == RTLOP_MULHU ? X86OP_UNARY_MUL_rAX:
                             /* RTLOP_MULHS */ X86OP_UNARY_IMUL_rAX);
            if (swapped_dx && !is_spilled(ctx, multiplier, insn_index)) {
                if (host_mult == X86_DX) {
                    host_mult = host_dest;
                } else if (host_mult == host_dest) {
                    host_mult = X86_DX;
                }
                append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev, opcode,
                                      host_mult);
            } else {
                append_insn_ModRM_ctx(&code, is64, X86OP_UNARY_Ev, opcode,
                                      ctx, insn_index, multiplier);
            }

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

          case RTLOP_DIVU:
          case RTLOP_DIVS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            ASSERT(host_dest != host_src2);
            ASSERT(host_dest != X86_DX);

            X86Register divisor = host_src2;

            /* As with MULH*, the presence of a temporary register means
             * we need to save the non-result output register.  For
             * division, that means either the register is live across this
             * instruction or it's assigned to src2 and we need to move
             * src2 out of the way of the dividend registers (rDX:rAX). */
            if (ctx->regs[dest].temp_allocated) {
                ASSERT(ctx->regs[dest].host_temp != X86_AX);
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      ctx->regs[dest].host_temp, X86_DX);
                if (divisor == X86_DX) {
                    divisor = ctx->regs[dest].host_temp;
                }
            }
            ASSERT(divisor != X86_DX);

            /* As with MULH*, if dest is not in the result register then
             * we need to save the result register's value.  For division,
             * we never allocate dest and src2 in the same register, so we
             * only need to check for host_dest == host_src1. */
            X86Register dividend = host_src1;
            if (host_dest != X86_AX) {
                if (dividend == host_dest) {
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          host_dest, X86_AX);
                    dividend = X86_AX;
                } else {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          host_dest, X86_AX);
                }
                /* RAX will be destroyed before the divide instruction, so
                 * make sure not to use it as the divisor in any case. */
                if (divisor == X86_AX) {
                    divisor = host_dest;
                }
            }
            ASSERT(divisor != X86_AX);

            if (is_spilled(ctx, src1, insn_index)) {
                append_load(&code, unit->regs[src1].type, X86_AX,
                            X86_SP, ctx->regs[src1].spill_offset);
            } else if (dividend != X86_AX) {
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      X86_AX, host_src1);
            }

            X86Opcode opcode;
            if (insn->opcode == RTLOP_DIVU) {
                append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                      X86_DX, X86_DX);
                opcode = X86OP_UNARY_DIV_rAX;
            } else {
                append_insn(&code, is64, X86OP_CWD);
                opcode = X86OP_UNARY_IDIV_rAX;
            }
            if (is_spilled(ctx, src2, insn_index)) {
                append_insn_ModRM_ctx(&code, is64, X86OP_UNARY_Ev, opcode,
                                      ctx, insn_index, src2);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev, opcode,
                                      divisor);
            }

            if (host_dest != X86_AX) {
                append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                      X86_AX, host_dest);
            }
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_DX, ctx->regs[dest].host_temp);
            }
            break;
          }  // case RTLOP_DIV[US]

          case RTLOP_MODU:
          case RTLOP_MODS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            ASSERT(host_dest != host_src2);
            ASSERT(host_dest != X86_AX);

            X86Register divisor = host_src2;

            if (ctx->regs[dest].temp_allocated) {
                ASSERT(ctx->regs[dest].host_temp != X86_DX);
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      ctx->regs[dest].host_temp, X86_AX);
                if (divisor == X86_AX) {
                    divisor = ctx->regs[dest].host_temp;
                }
            }
            ASSERT(divisor != X86_AX);

            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    X86_AX, src1);

            /* Save the result register's value if necesssary.  For modulo,
             * we take care of moving src1 to rAX first and we never
             * allocate dest and src2 in the same register, so this can
             * always be a regular MOV. */
            if (host_dest != X86_DX) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      host_dest, X86_DX);
                if (divisor == X86_DX) {
                    divisor = host_dest;
                }
            }
            ASSERT(divisor != X86_DX);

            X86Opcode opcode;
            if (insn->opcode == RTLOP_MODU) {
                append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                      X86_DX, X86_DX);
                opcode = X86OP_UNARY_DIV_rAX;
            } else {
                append_insn(&code, is64, X86OP_CWD);
                opcode = X86OP_UNARY_IDIV_rAX;
            }
            if (is_spilled(ctx, src2, insn_index)) {
                append_insn_ModRM_ctx(&code, is64, X86OP_UNARY_Ev, opcode,
                                      ctx, insn_index, src2);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev, opcode,
                                      divisor);
            }

            if (host_dest != X86_DX) {
                append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                      X86_DX, host_dest);
            }
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_AX, ctx->regs[dest].host_temp);
            }
            break;
          }  // case RTLOP_MOD[US]

          case RTLOP_SLL:
          case RTLOP_SRL:
          case RTLOP_SRA:
          case RTLOP_ROR: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool src2_spilled = is_spilled(ctx, src2, insn_index);
            ASSERT(host_dest != X86_CX);
            ASSERT(host_dest != host_src2 || src2_spilled);
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_SLL ? X86OP_SHIFT_SHL :
                insn->opcode == RTLOP_SRL ? X86OP_SHIFT_SHR :
                insn->opcode == RTLOP_SRA ? X86OP_SHIFT_SAR :
                             /* RTLOP_ROR */ X86OP_SHIFT_ROR);

            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    host_dest, src1);

            /* If we couldn't allocate rCX for the second operand, swap
             * it with whatever's in there now. This has to come after the
             * src1->dest copy! */
            bool swapped_cx = false;
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      ctx->regs[dest].host_temp, X86_CX);
            }
            if (src2_spilled) {
                /* If src2 is spilled but we don't have a temporary, rCX
                 * must be free, so we can just overwrite it.  If we do
                 * have a temporary, we just saved rCX away, so again we
                 * can just load straight into it.  For this specific case,
                 * we always use a 32-bit load even if src2 is a 64-bit
                 * value, since only the low 5-6 bits of the value matter.
                 * (x86 is little-endian, so we don't have to adjust the
                 * load address to do this.) */
                append_load(&code, RTLTYPE_INT32, X86_CX,
                            X86_SP, ctx->regs[src1].spill_offset);
            } else if (host_src2 != X86_CX) {
                append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                      X86_CX, host_src2);
                swapped_cx = true;
            }

            append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_CL,
                                  opcode, host_dest);

            /* If we had to save or swap rCX, restore the original register
             * values.  But prefer MOV over XCHG (and discard src2) if src2
             * dies on this instruction, since MOV can be zero-latency. */
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_CX, ctx->regs[dest].host_temp);
            } else if (swapped_cx) {
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
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            if (handle->setup.host_features & BINREC_FEATURE_X86_LZCNT) {
                append_insn_ModRM_ctx(&code, is64, X86OP_LZCNT, host_dest,
                                      ctx, insn_index, src1);
            } else {
                ASSERT(ctx->regs[dest].temp_allocated);
                const X86Register host_temp = ctx->regs[dest].host_temp;
                append_insn_ModRM_ctx(&code, is64, X86OP_BSR, host_dest,
                                      ctx, insn_index, src1);
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
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            if (is_spilled(ctx, src1, insn_index)) {
                append_load(&code, unit->regs[src1].type, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
            } else if (host_dest != host_src1) {
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Ev_Gv,
                                      host_src1, host_dest);
            }
            append_insn_R(&code, is64, X86OP_BSWAP_rAX, host_dest);
            break;
          }  // case RTLOP_BSWAP

          case RTLOP_SEQ:
          case RTLOP_SLTU:
          case RTLOP_SLTS:
          case RTLOP_SGTU:
          case RTLOP_SGTS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode set_opcode = (
                insn->opcode == RTLOP_SLTU ? X86OP_SETB :
                insn->opcode == RTLOP_SLTS ? X86OP_SETL :
                insn->opcode == RTLOP_SGTU ? X86OP_SETA :
                insn->opcode == RTLOP_SGTS ? X86OP_SETG :
                             /* RTLOP_SEQ */ X86OP_SETZ);
            if (is_spilled(ctx, src1, insn_index)) {
                append_load(&code, unit->regs[src1].type, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_src1 = host_dest;
            }
            append_insn_ModRM_ctx(&code, is64, X86OP_CMP_Gv_Ev, host_src1,
                                  ctx, insn_index, src2);
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
          }  // case RTLOP_{SEQ,SLTU,SLTS,SGTU,SGTS}

          case RTLOP_BFEXT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);

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
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_dest);
                append_imm8(&code, insn->bitfield.start);
                host_shifted = host_dest;
            } else if (is_spilled(ctx, src1, insn_index)) {
                append_load(&code, unit->regs[src1].type, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
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
            const bool src2_spilled = is_spilled(ctx, src2, insn_index);
            ASSERT(host_dest != host_src2 || src2_spilled);
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
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
                if (host_dest == host_src1
                 && !is_spilled(ctx, src1, insn_index)) {
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
                    append_insn_ModRM_ctx(&code, true, X86OP_AND_Gv_Ev,
                                          host_dest, ctx, insn_index, src1);
                }
            } else {
                const uint32_t src2_mask = (1U << insn->bitfield.count) - 1;
                const uint32_t src1_mask = ~(src2_mask << insn->bitfield.start);
                if (src1_mask == 0x000000FF) {
                    append_insn_ModRM_ctx(&code, is64, X86OP_MOVZX_Gv_Eb,
                                          host_dest, ctx, insn_index, src1);
                } else if (src1_mask == 0x0000FFFF) {
                    append_insn_ModRM_ctx(&code, is64, X86OP_MOVZX_Gv_Ew,
                                          host_dest, ctx, insn_index, src1);
                } else {
                    append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                            host_dest, src1);
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
             * the temporary register if it's not spilled and it dies on
             * this instruction. */
            X86Register host_newbits;
            if (!src2_spilled && unit->regs[src2].death == insn_index) {
                host_newbits = host_src2;
            } else {
                ASSERT(ctx->regs[dest].temp_allocated);
                const X86Register host_temp = ctx->regs[dest].host_temp;
                /* This assertion will hold even if src2 is currently
                 * spilled, because whatever register is occupying
                 * host_src2 must be live past this instruction. */
                ASSERT(host_temp != host_src2);
                host_newbits = host_temp;
            }
            if (insn->bitfield.count > 32) {
                /* We can't use a 64-bit immediate value with AND, so
                 * shift the value left and (if necessary) right again. */
                ASSERT(is64);
                if (host_newbits != host_src2) {
                    append_insn_ModRM_ctx(&code, true, X86OP_MOV_Gv_Ev,
                                          host_newbits, ctx, insn_index, src2);
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
                        append_insn_ModRM_ctx(
                            &code, is64, X86OP_MOV_Gv_Ev, host_newbits,
                            ctx, insn_index, src2);
                    }
                } else if (insn->bitfield.count < 8) {
                    if (host_newbits != host_src2) {
                        /* This can safely be a 32-bit move even if src2
                         * is a spilled 64-bit value, since x86 is
                         * little-endian. */
                        append_insn_ModRM_ctx(
                            &code, false, X86OP_MOV_Gv_Ev, host_newbits,
                            ctx, insn_index, src2);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                          X86OP_IMM_AND, host_newbits);
                    append_imm8(&code, (1U << insn->bitfield.count) - 1);
                } else if (insn->bitfield.count == 8) {
                    /* Registers SP-DI require an empty REX prefix to
                     * access the low byte as a byte register, but be
                     * careful not to double REX if a prefix will already
                     * be added. */
                    if (!src2_spilled
                     && host_src2 >= X86_SP && host_src2 <= X86_DI
                     && host_newbits <= X86_DI) {
                        append_opcode(&code, X86OP_REX);
                    }
                    append_insn_ModRM_ctx(
                        &code, false, X86OP_MOVZX_Gv_Eb, host_newbits,
                        ctx, insn_index, src2);
                } else if (insn->bitfield.count == 16) {
                    append_insn_ModRM_ctx(
                        &code, false, X86OP_MOVZX_Gv_Ew, host_newbits,
                        ctx, insn_index, src2);
                } else if (insn->bitfield.count == 32) {
                    append_insn_ModRM_ctx(
                        &code, false, X86OP_MOV_Gv_Ev, host_newbits,
                        ctx, insn_index, src2);
                } else {
                    if (host_newbits != host_src2) {
                        append_insn_ModRM_ctx(
                            &code, false, X86OP_MOV_Gv_Ev, host_newbits,
                            ctx, insn_index, src2);
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

          case RTLOP_ADDI:
          case RTLOP_ANDI:
          case RTLOP_ORI:
          case RTLOP_XORI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            /* The immediate value is actually signed, but we treat it as
             * unsigned here to simplify range testing. */
            const uint32_t imm = (uint32_t)insn->src_imm;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_ADDI ? X86OP_IMM_ADD :
                insn->opcode == RTLOP_ANDI ? X86OP_IMM_AND :
                insn->opcode == RTLOP_ORI ? X86OP_IMM_OR :
                             /* RTLOP_XORI */ X86OP_IMM_XOR);
            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            if (imm + 128 < 256) {
                append_insn_ModRM_reg(&code, is64, X86OP_IMM_Ev_Ib,
                                      opcode, host_dest);
                append_imm8(&code, (uint8_t)imm);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_IMM_Ev_Iz,
                                      opcode, host_dest);
                append_imm32(&code, imm);
            }
            break;
          }  // case RTLOP_{ADDI,ANDI,ORI,XORI}

          case RTLOP_MULI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const uint32_t imm = (uint32_t)insn->src_imm;  // As for ADDI etc.
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            if (imm + 128 < 256) {
                append_insn_ModRM_ctx(&code, is64, X86OP_IMUL_Gv_Ev_Ib,
                                      host_dest, ctx, insn_index, src1);
                append_imm8(&code, (uint8_t)imm);
            } else {
                append_insn_ModRM_ctx(&code, is64, X86OP_IMUL_Gv_Ev_Iz,
                                      host_dest, ctx, insn_index, src1);
                append_imm32(&code, imm);
            }
            break;
          }  // case RTLOP_MULI

          case RTLOP_SLLI:
          case RTLOP_SRLI:
          case RTLOP_SRAI:
          case RTLOP_RORI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const uint8_t shift_count = (uint8_t)insn->src_imm;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_SLLI ? X86OP_SHIFT_SHL :
                insn->opcode == RTLOP_SRLI ? X86OP_SHIFT_SHR :
                insn->opcode == RTLOP_SRAI ? X86OP_SHIFT_SAR :
                             /* RTLOP_RORI */ X86OP_SHIFT_ROR);
            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_Ib,
                                  opcode, host_dest);
            append_imm8(&code, shift_count);
            break;
          }  // case RTLOP_{SLLI,SRLI,SRAI,RORI}

          case RTLOP_SEQI:
          case RTLOP_SLTUI:
          case RTLOP_SLTSI:
          case RTLOP_SGTUI:
          case RTLOP_SGTSI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const uint32_t imm = (uint32_t)insn->src_imm;  // As for ADDI etc.
            const bool is64 = (unit->regs[src1].type == RTLTYPE_ADDRESS);
            const X86Opcode set_opcode = (
                insn->opcode == RTLOP_SLTUI ? X86OP_SETB :
                insn->opcode == RTLOP_SLTSI ? X86OP_SETL :
                insn->opcode == RTLOP_SGTUI ? X86OP_SETA :
                insn->opcode == RTLOP_SGTSI ? X86OP_SETG :
                             /* RTLOP_SEQI */ X86OP_SETZ);

            if (imm + 128 < 256) {
                append_insn_ModRM_ctx(
                    &code, is64, X86OP_IMM_Ev_Ib, X86OP_IMM_CMP,
                    ctx, insn_index, src1);
                append_imm8(&code, (uint8_t)imm);
            } else {
                append_insn_ModRM_ctx(
                    &code, is64, X86OP_IMM_Ev_Iz, X86OP_IMM_CMP,
                    ctx, insn_index, src1);
                append_imm32(&code, imm);
            }

            /* Handle empty REX prefix for low byte of SP-DI. */
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
          }  // case RTLOP_{SEQI,SLTUI,SLTSI,SLEUI,SLESI}

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
            ASSERT(host_src_i >= 0);  // Checked during register allocation.
            const X86Register host_src = (X86Register)host_src_i;
            if (host_dest != host_src) {
                const bool is64 = (unit->regs[dest].type == RTLTYPE_ADDRESS);
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src);
            }
            break;
          }  // case RTLOP_LOAD_ARG

          case RTLOP_LOAD: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_base;
            if (is_spilled(ctx, src1, insn_index)) {
                ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
                append_load(&code, RTLTYPE_ADDRESS, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_base = host_dest;
            } else {
                host_base = ctx->regs[src1].host_reg;
            }
            append_load(&code, unit->regs[dest].type, host_dest,
                        host_base, insn->offset);
            break;
          }  // case RTLOP_LOAD

          case RTLOP_LOAD_U8:
          case RTLOP_LOAD_S8: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_base;
            if (is_spilled(ctx, src1, insn_index)) {
                ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
                append_load(&code, RTLTYPE_ADDRESS, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_base = host_dest;
            } else {
                host_base = ctx->regs[src1].host_reg;
            }
            const X86Opcode opcode = (insn->opcode == RTLOP_LOAD_U8
                                      ? X86OP_MOVZX_Gv_Eb : X86OP_MOVSX_Gv_Eb);
            append_insn_ModRM_mem(&code, false, opcode,
                                  host_dest, host_base, insn->offset);
            break;
          }  // case RTLOP_LOAD_U8, RTLOP_LOAD_S8

          case RTLOP_LOAD_U16:
          case RTLOP_LOAD_S16: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_base;
            if (is_spilled(ctx, src1, insn_index)) {
                ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
                append_load(&code, RTLTYPE_ADDRESS, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_base = host_dest;
            } else {
                host_base = ctx->regs[src1].host_reg;
            }
            const X86Opcode opcode = (insn->opcode == RTLOP_LOAD_U16
                                      ? X86OP_MOVZX_Gv_Ew : X86OP_MOVSX_Gv_Ew);
            append_insn_ModRM_mem(&code, false, opcode,
                                  host_dest, host_base, insn->offset);
            break;
          }  // case RTLOP_LOAD_U16, RTLOP_LOAD_S16

          case RTLOP_STORE:
            append_store(&code, unit->regs[src2].type, ctx->regs[src2].host_reg,
                         ctx->regs[src1].host_reg, insn->offset);
            break;

          case RTLOP_STORE_I8: {
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            /* Handle mandatory REX prefix for low byte of SP/BP/SI/DI. */
            if (host_src2 >= X86_SP && host_src2 <= X86_DI
             && host_src1 <= X86_DI) {
                append_opcode(&code, X86OP_REX);
            }
            append_insn_ModRM_mem(&code, false, X86OP_MOV_Eb_Gb,
                                  host_src2, host_src1, insn->offset);
            break;
          }  // case RTLOP_STORE_I8

          case RTLOP_STORE_I16: {
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            append_opcode(&code, X86OP_OPERAND_SIZE);
            append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                  host_src2, host_src1, insn->offset);
            break;
          }  // case RTLOP_STORE_I16

          case RTLOP_LOAD_BR: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_base;
            if (is_spilled(ctx, src1, insn_index)) {
                ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
                append_load(&code, RTLTYPE_ADDRESS, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_base = host_dest;
            } else {
                host_base = ctx->regs[src1].host_reg;
            }
            switch (unit->regs[dest].type) {
              case RTLTYPE_INT32:
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, false, X86OP_MOVBE_Gy_My,
                                          host_dest, host_base, insn->offset);
                } else {
                    append_insn_ModRM_mem(&code, false, X86OP_MOV_Gv_Ev,
                                          host_dest, host_base, insn->offset);
                    append_insn_R(&code, false, X86OP_BSWAP_rAX, host_dest);
                }
                break;
              case RTLTYPE_ADDRESS:
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, true, X86OP_MOVBE_Gy_My,
                                          host_dest, host_base, insn->offset);
                } else {
                    append_insn_ModRM_mem(&code, true, X86OP_MOV_Gv_Ev,
                                          host_dest, host_base, insn->offset);
                    append_insn_R(&code, true, X86OP_BSWAP_rAX, host_dest);
                }
                break;
              default:
                // FIXME: handle FP registers when we support those
                ASSERT(!"Invalid data type in LOAD_BR");
            }
            break;
          }  // case RTLOP_LOAD_BR

          case RTLOP_LOAD_U16_BR:
          case RTLOP_LOAD_S16_BR: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_base;
            if (is_spilled(ctx, src1, insn_index)) {
                ASSERT(unit->regs[src1].type == RTLTYPE_ADDRESS);
                append_load(&code, RTLTYPE_ADDRESS, host_dest,
                            X86_SP, ctx->regs[src1].spill_offset);
                host_base = host_dest;
            } else {
                host_base = ctx->regs[src1].host_reg;
            }

            if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                append_insn_ModRM_mem(&code, false, X86OP_MOVBE_Gw_Mw,
                                      host_dest, host_base, insn->offset);
            } else {
                /* MOVZX instead of plain MOV (which would leave the high
                 * bits of the destination register unchanged) to avoid a
                 * false dependency on the previous value of the register. */
                append_insn_ModRM_mem(&code, false, X86OP_MOVZX_Gv_Ew,
                                      host_dest, host_base, insn->offset);
                /* rorw $8,%reg would be slightly more compact, but that
                 * incurs both a rotate penalty and a partial register
                 * stall when extending to 32 bits below.  The byte-XCHG
                 * idiom (e.g. XCHG AH,AL) similarly seems likely to cause
                 * a partial register stall, so we don't do that either.
                 * Modern processors should all support MOVBE anyway. */
                append_insn_R(&code, false, X86OP_BSWAP_rAX, host_dest);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_dest);
                append_imm8(&code, 16);
                if (insn->opcode == RTLOP_LOAD_U16_BR) {
                    break;  // Already zero-extended.
                }
            }

            const X86Opcode opcode = (insn->opcode == RTLOP_LOAD_U16_BR
                                      ? X86OP_MOVZX_Gv_Ew : X86OP_MOVSX_Gv_Ew);
            append_insn_ModRM_reg(&code, false, opcode, host_dest, host_dest);
            break;
          }  // case RTLOP_LOAD_U16_BR, RTLOP_LOAD_S16_BR

          case RTLOP_STORE_BR: {
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            switch (unit->regs[src2].type) {
              case RTLTYPE_INT32:
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, false, X86OP_MOVBE_My_Gy,
                                          host_src2, host_src1, insn->offset);
                } else {
                    append_insn_R(&code, false, X86OP_BSWAP_rAX, host_src2);
                    append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                          host_src2, host_src1, insn->offset);
                    if (unit->regs[src2].death > insn_index) {
                        append_insn_R(&code, false, X86OP_BSWAP_rAX, host_src2);
                    }
                }
                break;
              case RTLTYPE_ADDRESS:
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, true, X86OP_MOVBE_My_Gy,
                                          host_src2, host_src1, insn->offset);
                } else {
                    append_insn_R(&code, true, X86OP_BSWAP_rAX, host_src2);
                    append_insn_ModRM_mem(&code, true, X86OP_MOV_Ev_Gv,
                                          host_src2, host_src1, insn->offset);
                    if (unit->regs[src2].death > insn_index) {
                        append_insn_R(&code, true, X86OP_BSWAP_rAX, host_src2);
                    }
                }
                break;
              default:
                // FIXME: handle FP registers when we support those
                ASSERT(!"Invalid data type in STORE_BR");
            }
            break;
          }  // case RTLOP_STORE_BR

          case RTLOP_STORE_I16_BR: {
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                append_insn_ModRM_mem(&code, false, X86OP_MOVBE_Mw_Gw,
                                      host_src2, host_src1, insn->offset);
            } else if (unit->regs[src2].death <= insn_index) {
                append_insn_R(&code, false, X86OP_BSWAP_rAX, host_src2);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_src2);
                append_imm8(&code, 16);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                      host_src2, host_src1, insn->offset);
            } else {
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_ROR, host_src2);
                append_imm8(&code, 8);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                      host_src2, host_src1, insn->offset);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_ROR, host_src2);
                append_imm8(&code, 8);
            }
            break;
          }  // case RTLOP_STORE_I16_BR

          case RTLOP_LABEL:
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(ctx->label_offsets[insn->label] < 0);
            ctx->label_offsets[insn->label] = code.len;
            break;

          case RTLOP_GOTO:
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(block_info->unresolved_branch_offset < 0);
            ASSERT(unit->label_blockmap[insn->label] >= 0);
            if (!setup_aliases_for_block(&code, ctx, block_index,
                                         unit->label_blockmap[insn->label])) {
                return false;
            }
            append_jump(&code, block_info, X86OP_JMP_Jb, X86OP_JMP_Jz,
                        insn->label, ctx->label_offsets[insn->label]);
            fall_through = false;
            break;

          case RTLOP_GOTO_IF_Z:
          case RTLOP_GOTO_IF_NZ: {
            const X86Opcode short_opcode =
                (insn->opcode == RTLOP_GOTO_IF_Z ? X86OP_JZ_Jb : X86OP_JNZ_Jb);
            const X86Opcode long_opcode =
                (insn->opcode == RTLOP_GOTO_IF_Z ? X86OP_JZ_Jz : X86OP_JNZ_Jz);

            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(block_info->unresolved_branch_offset < 0);
            const int target_block = unit->label_blockmap[insn->label];
            ASSERT(target_block >= 0);

            append_test_reg(ctx, unit, insn_index, &code, src1);

            /* If we have any aliases to load that would conflict with live
             * registers, we have to invert the sense of the branch here
             * and set up the registers conditionally. */
            if (check_alias_conflicts(ctx, block_index, target_block)) {
                uint8_t setup_buffer[SETUP_ALIAS_SIZE];
                CodeBuffer setup_code = {.buffer = setup_buffer,
                                         .buffer_size = sizeof(setup_buffer),
                                         .len = 0};
                ASSERT(setup_aliases_for_block(&setup_code, ctx, block_index,
                                               target_block));
                /* Flipping the low bit of the opcode will invert the sense
                 * of the branch. */
                const long setup_jump = code.len;
                /* Write this jump as though the next one (to the target
                 * label) will have a 32-bit displacement.  If it ends up
                 * having an 8-bit displacement, we'll fix up this
                 * instruction afterwards. */
                append_jump_raw(&code, short_opcode ^ 1, long_opcode ^ 1,
                                setup_code.len + 5);
                const long setup_start = code.len;
                const long needed_space = setup_code.len + 5;
                if (UNLIKELY(code.len + needed_space > code.buffer_size)) {
                    handle->code_len = code.len;
                    if (UNLIKELY(!binrec_ensure_code_space(
                                     handle, needed_space))) {
                        return false;
                    }
                    code.buffer = handle->code_buffer;
                    code.buffer_size = handle->code_buffer_size;
                }
                memcpy(&code.buffer[code.len], setup_code.buffer,
                       setup_code.len);
                code.len += setup_code.len;
                const long final_jump = code.len;
                append_jump(&code, block_info, X86OP_JMP_Jb, X86OP_JMP_Jz,
                            insn->label, ctx->label_offsets[insn->label]);
                if (code.len == final_jump + 2) {
                    /* In order for the initial (conditional) jump over the
                     * setup code to have a 32-bit displacement, the setup
                     * code must have been at least 123 bytes long.  But in
                     * that case, the displacement for the final jump will
                     * be -6 (for the initial jump) - 123 - at least 2 for
                     * this jump, which is less than -128 so it can't be
                     * encoded in one byte.  Thus, if the final jump has an
                     * 8-bit displacement, the initial jump must also have
                     * had an 8-bit displacement. */
                    ASSERT(setup_start == setup_jump + 2);
                    code.buffer[setup_start - 1] -= 3;
                }
                initial_len = code.len;  // Suppress output length check.
            } else {
                if (!setup_aliases_for_block(&code, ctx, block_index,
                                             target_block)) {
                    return false;
                }
                append_jump(&code, block_info, short_opcode, long_opcode,
                            insn->label, ctx->label_offsets[insn->label]);
            }
            break;
          }  // case RTLOP_GOTO_IF_Z, RTLOP_GOTO_IF_NZ

          case RTLOP_RETURN:
            ASSERT(block_info->unresolved_branch_offset < 0);
            if (src1) {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        X86_AX, src1);
            }
            /* We use label 0 (normally invalid) to indicate a jump to the
             * function epilogue. */
            append_jump(&code, block_info, X86OP_JMP_Jb, X86OP_JMP_Jz, 0, -1);
            fall_through = false;
            break;

          case RTLOP_ILLEGAL:
            append_opcode(&code, X86OP_UD2);
            break;

        }

        ASSERT(code.len - initial_len <= MAX_INSN_LEN);
    }

    if (fall_through && block->next_block >= 0) {
        if (!setup_aliases_for_block(&code, ctx, block_index,
                                     block->next_block)) {
            return false;
        }
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
 *     True on success, false if out of memory.
 */
static bool append_prologue(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);

    binrec_t * const handle = ctx->handle;
    const uint32_t regs_to_save = ctx->regs_touched & ctx->callee_saved_regs;
    const bool is_windows_seh =
        (ctx->handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);

    /* Figure out how much stack space we use in total. */
    int total_stack_use;
    const int push_size = 8 * popcnt32(regs_to_save & 0xFFFF);
    total_stack_use = push_size;
    /* If we have any XMM registers to save, we have to align the stack at
     * this point so the saves and loads are properly aligned.  This implies
     * that we also need to align the frame size here, since the final stack
     * pointer must remain 16-byte aligned. */
    const uint32_t xmm_to_save = regs_to_save >> 16;
    if (xmm_to_save) {
        /* The stack pointer after pushes is either 0 or 8 bytes past a
         * multiple of 16.  To align it, we subtract 8 if the number of
         * pushes is even.  (That's not a typo -- the stack pointer comes
         * in unaligned due to the return address pushed by the CALL
         * instruction that jumped here.) */
        if (push_size % 16 == 0) {
            total_stack_use += 8;
        }
        total_stack_use += 16 * popcnt32(xmm_to_save);
        ctx->frame_size = align_up(ctx->frame_size, 16);
    }
    total_stack_use += ctx->frame_size;
    /* Final stack pointer alignment: the total stack usage should be a
     * multiple of 16 plus 8, again because of the return address. */
    if (total_stack_use % 16 != 8) {
        total_stack_use += 16 - ((total_stack_use + 8) & 15);
    }

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
            if (regs_to_save & (1u << reg)) {
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
            if (regs_to_save & (1u << reg)) {
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
        if (regs_to_save & (1u << reg)) {
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
        if (regs_to_save & (1u << reg)) {
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
 *     True on success, false if out of memory.
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
        if (regs_saved & (1u << reg)) {
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
        if (regs_saved & (1u << reg)) {
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
        if (branch_offset >= 0) {
            const int label = block_info->unresolved_branch_target;
            ASSERT(label >= 0 && label < ctx->unit->next_label);
            ASSERT(ctx->label_offsets[label] >= 0);
            long offset = ctx->label_offsets[label] - branch_offset;
            ASSERT(offset > 0);  // Or else it would have been resolved.
            ASSERT(offset < INT64_C(0x80000000));  // Sanity check.
            uint8_t *ptr = &ctx->handle->code_buffer[branch_offset];
            offset -= 4;
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
 *     True on success, false if out of memory.
 */
static bool translate_unit(HostX86Context *ctx)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);

    if (!append_prologue(ctx)) {
        return false;
    }

    memset(ctx->reg_map, 0, sizeof(ctx->reg_map));
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
    binrec_free(ctx->handle, ctx->alias_buffer);
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
    ctx->alias_buffer = binrec_malloc(
        handle, ((4 * unit->next_alias) * unit->num_blocks));
    if (!ctx->blocks || !ctx->regs || !ctx->label_offsets
     || !ctx->alias_buffer) {
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
    memset(ctx->alias_buffer, 0, ((4 * unit->next_alias) * unit->num_blocks));

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
        log_error(handle, "Out of memory while generating code");
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
