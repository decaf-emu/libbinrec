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
/************* Local constant and data structure definitions *************/
/*************************************************************************/

/* Table of local constant data to insert in the prologue. */
static const struct {
    uint8_t data[16];
} local_constants[NUM_LOCAL_CONSTANTS] = {
    [LC_FLOAT32_SIGNBIT       ] = {{0x00,0x00,0x00,0x80}},
    [LC_FLOAT32_INV_SIGNBIT   ] = {{0xFF,0xFF,0xFF,0x7F}},
    [LC_FLOAT64_SIGNBIT       ] = {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80}},
    [LC_FLOAT64_INV_SIGNBIT   ] = {{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F}},
    [LC_V2_FLOAT32_SIGNBIT    ] = {{0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80}},
    [LC_V2_FLOAT32_INV_SIGNBIT] = {{0xFF,0xFF,0xFF,0x7F,0xFF,0xFF,0xFF,0x7F}},
    [LC_V2_FLOAT64_SIGNBIT    ] = {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
                                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80}},
    [LC_V2_FLOAT64_INV_SIGNBIT] = {{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F,
                                    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F}},
    [LC_V2_FLOAT32_HIGH_ONES  ] = {{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                    0x00,0x00,0x80,0x3F,0x00,0x00,0x80,0x3F}},
};

/*-----------------------------------------------------------------------*/

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
/************************ Basic utility routines *************************/
/*************************************************************************/

/**
 * current_reg:  Helper function to return the RTL register currently
 * occupying the given host register.  Returns zero if ctx->reg_map[] has
 * a nonzero entry but that register is already dead, or if host_reg is
 * occupied by the destination of the current instruction and that
 * register dies immediately (a dead store).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     host_reg: Host register to look up (X86Register).
 * [Return value]
 *     Index of the RTL register using the given host register, or 0 if
 *     the host register is not in use.
 */
static inline PURE_FUNCTION int current_reg(
    const HostX86Context *ctx, int insn_index, X86Register host_reg)
{
    const int rtl_reg = ctx->reg_map[host_reg];
    if (rtl_reg && ctx->unit->regs[rtl_reg].death > insn_index) {
        return rtl_reg;
    } else {
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * is_spilled:  Helper function to return whether a register is currently
 * spilled.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     reg: RTL register number.
 * [Return value]
 *     True if the register is spilled at insn_index, false if not.
 */
static inline PURE_FUNCTION bool is_spilled(
    const HostX86Context *ctx, int insn_index, int reg)
{
    const HostX86RegInfo *reg_info = &ctx->regs[reg];
    return reg_info->spilled && reg_info->spill_insn <= insn_index;
}

/*-----------------------------------------------------------------------*/

/**
 * sse_opcode_prefix_for_type:  Return the opcode prefix byte (0x66, 0xF3,
 * 0xF2, or 0 for no prefix) to use with SSE opcodes for the given data type.
 *
 * [Parameters]
 *     type: RTL data type.
 * [Return value]
 *     Prefix byte, or 0 for no prefix.
 */
static inline CONST_FUNCTION uint8_t sse_opcode_prefix_for_type(
    RTLDataType type)
{
    switch (type) {
        case RTLTYPE_V2_FLOAT64: return 0x66;
        case RTLTYPE_FLOAT32:    return 0xF3;
        case RTLTYPE_FLOAT64:    return 0xF2;
        default:                 return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * vex_pp_for_opcode_prefix:  Return the 2-bit VEX "pp" field value
 * corresponding to the given opcode prefix byte (0x66, 0xF3, 0xF2, or
 * 0 for no prefix).
 *
 * [Parameters]
 *     prefix: Prefix byte, or 0 for no prefix.
 * [Return value]
 *     VEX.pp value corresponding to the prefix.
 */
static inline CONST_FUNCTION uint8_t vex_pp_for_opcode_prefix(uint8_t prefix)
{
    switch (prefix) {
        case 0x66: return 1;
        case 0xF3: return 2;
        case 0xF2: return 3;
        default: ASSERT(prefix == 0); return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * rtlfexc_to_bits:  Return an MXCSR bitmask corresponding to the
 * exception(s) specified by exc, or 0 if exc is invalid.
 */
static inline CONST_FUNCTION uint8_t rtlfexc_to_bits(RTLFloatException exc)
{
    switch (exc) {
        case RTLFEXC_ANY:         return 0x3D;
        case RTLFEXC_INEXACT:     return 0x20;
        case RTLFEXC_INVALID:     return 0x01;
        case RTLFEXC_OVERFLOW:    return 0x08;
        case RTLFEXC_UNDERFLOW:   return 0x10;
        case RTLFEXC_ZERO_DIVIDE: return 0x04;
    }
    return 0;
}

/*************************************************************************/
/*************** Utility routines for adding instructions ****************/
/*************************************************************************/

/**
 * append_opcode:  Append an x86 opcode to the current code stream.  The
 * code buffer is assumed to have enough space for the instruction.
 */
static inline void append_opcode(CodeBuffer *code, X86Opcode opcode)
{
    uint8_t *ptr = code->buffer + code->len;

    if ((uint32_t)opcode <= 0xFF) {
        ASSERT(code->len + 1 <= code->buffer_size);
        code->len += 1;
        *ptr++ = opcode;
    } else if ((uint32_t)opcode <= 0xFFFF) {
        ASSERT(code->len + 2 <= code->buffer_size);
        code->len += 2;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else if ((uint32_t)opcode <= 0xFFFFFF) {
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
 * append_rex_opcode:  Append an x86 opcode with a REX prefix to the
 * current code stream.  The code buffer is assumed to have enough space
 * for the instruction.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     rex: REX flags (bitwise OR of X86_REX_* or X86OP_REX_*).
 *     opcode: Opcode to append.
 */
static inline void append_rex_opcode(CodeBuffer *code, uint8_t rex,
                                     X86Opcode opcode)
{
    uint8_t *ptr = code->buffer + code->len;
    rex |= X86OP_REX;

    if ((uint32_t)opcode <= 0xFF) {
        ASSERT(code->len + 2 <= code->buffer_size);
        code->len += 2;
        *ptr++ = rex;
        *ptr++ = opcode;
    } else if ((uint32_t)opcode <= 0xFFFF) {
        ASSERT(code->len + 3 <= code->buffer_size);
        code->len += 3;
        *ptr++ = rex;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    } else if ((uint32_t)opcode <= 0xFFFFFF) {
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
        ASSERT((uint32_t)opcode>>24 == 0x66
            || (uint32_t)opcode>>24 == 0xF2
            || (uint32_t)opcode>>24 == 0xF3);
        *ptr++ = opcode >> 24;
        *ptr++ = rex;
        *ptr++ = opcode >> 16;
        *ptr++ = opcode >> 8;
        *ptr++ = opcode;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * maybe_append_empty_rex:  Append an empty REX prefix (0x40) if
 * host_bytereg is one of X86_SP, X86_BP, X86_SI, or X86_DI and both
 * host_other1 and host_other2 are -1 (indicating no register) or one of
 * X86_AX through X86_DI.  These are the conditions under which a REX
 * prefix is required (even if empty) to access host_bytereg as a byte
 * register; without REX, the corresponding values for the register field
 * in the opcode map to AH through DH instead.
 */
static inline void maybe_append_empty_rex(
    CodeBuffer *code, int host_bytereg, int host_other1, int host_other2)
{
    if (host_bytereg >= X86_SP && host_bytereg <= X86_DI
     && host_other1 <= X86_DI && host_other2 <= X86_DI) {
        append_opcode(code, X86OP_REX);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_vex_opcode:  Append an x86 opcode with a VEX prefix to the
 * current code stream.  The code buffer is assumed to have enough space
 * for the instruction.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     opcode: Opcode to append.
 *     vex_W: True to set the W field of the VEX prefix.
 *     vex_L: True to set the L field of the VEX prefix.
 *     vex_R: True to set the R field of the VEX prefix (to 0).
 *     vex_X: True to set the X field of the VEX prefix (to 0).
 *     vex_B: True to set the B field of the VEX prefix (to 0).
 *     vex_vvvv: Register number for the vvvv field of the VEX prefix,
 *         _not_ complemented.  Pass 0 (not 15) if the field is not used.
 */
static inline void append_vex_opcode(
    CodeBuffer *code, X86Opcode opcode, bool vex_W, bool vex_L, bool vex_R,
    bool vex_X, bool vex_B, int vex_vvvv)
{
    ASSERT((uint32_t)opcode > 0xFF);

    uint8_t *ptr = code->buffer + code->len;

    /* Currently we only use this function with instructions using the
     * 0F 38 escape bytes, so we always use the 3-byte VEX prefix.  For
     * plain 0F opcodes, we could potentially use the 2-byte VEX format
     * instead. */
    ASSERT((opcode & 0xFFFFFF) >= 0x0F3800 && (opcode & 0xFFFFFF) <= 0x0F38FF);
    const uint8_t prefix_byte = opcode >> 24;
    const uint8_t vex_pp = vex_pp_for_opcode_prefix(prefix_byte);

    ASSERT(code->len + 4 <= code->buffer_size);
    code->len += 4;
    *ptr++ = X86OP_VEX3;
    *ptr++ = ((vex_R<<7 | vex_X<<6 | vex_B<<5) ^ 0xE0) | 0x02;
    *ptr++ = vex_W<<7 | (~vex_vvvv & 15) << 3 | vex_L<<2 | vex_pp;
    *ptr++ = opcode & 0xFF;
}

/*-----------------------------------------------------------------------*/

/**
 * append_imm8:  Append an 8-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_imm8(CodeBuffer *code, uint8_t value)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 1 <= code->buffer_size);
    code->len += 1;
    *ptr++ = value;
}

/*-----------------------------------------------------------------------*/

/**
 * append_imm16:  Append a 16-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_imm16(CodeBuffer *code, uint16_t value)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 2 <= code->buffer_size);
    code->len += 2;
    *ptr++ = (uint8_t)(value >> 0);
    *ptr++ = (uint8_t)(value >> 8);
}

/*-----------------------------------------------------------------------*/

/**
 * append_imm32:  Append a 32-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_imm32(CodeBuffer *code, uint32_t value)
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
 * append_imm64:  Append a 64-bit immediate value to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_imm64(CodeBuffer *code, uint64_t value)
{
    uint8_t *ptr = code->buffer + code->len;

    ASSERT(code->len + 8 <= code->buffer_size);
    code->len += 8;
    *ptr++ = (uint8_t)(value >>  0);
    *ptr++ = (uint8_t)(value >>  8);
    *ptr++ = (uint8_t)(value >> 16);
    *ptr++ = (uint8_t)(value >> 24);
    *ptr++ = (uint8_t)(value >> 32);
    *ptr++ = (uint8_t)(value >> 40);
    *ptr++ = (uint8_t)(value >> 48);
    *ptr++ = (uint8_t)(value >> 56);
}

/*-----------------------------------------------------------------------*/

/**
 * append_ModRM:  Append a ModR/M byte to the current code stream.
 * The code buffer is assumed to have enough space.
 */
static inline void append_ModRM(CodeBuffer *code, X86Mod mod,
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
static inline void append_ModRM_SIB(
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
 *     is64: True to prepend REX.W to an integer instruction, false otherwise.
 *     opcode: Instruction opcode.
 */
static inline void append_insn(CodeBuffer *code, bool is64, X86Opcode opcode)
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
 *     is64: True to prepend REX.W to an integer instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg: Register.
 */
static inline void append_insn_R(
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
 *     is64: True to prepend REX.W to an integer instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg1: Register or sub-opcode for ModR/M reg field.
 *     reg2: Register for ModR/M r/m field.
 */
static inline void append_insn_ModRM_reg(
    CodeBuffer *code, bool is64, X86Opcode opcode, int reg1, X86Register reg2)
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
 * encoding a memory EA in the instruction.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True to prepend REX.W to an integer instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg: Register or sub-opcode for ModR/M reg field.
 *     base: Base register for memory address.
 *     index: Index register for memory address, or -1 to omit the index.
 *         Must not be X86_SP.
 *     offset: Constant offset for memory address.
 */
static inline void append_insn_ModRM_mem(
    CodeBuffer *code, bool is64, X86Opcode opcode, int reg,
    X86Register base, int index, int32_t offset)
{
    uint8_t rex = is64 ? X86_REX_W : 0;
    if (reg & 8) {
        rex |= X86_REX_R;
    }
    if (index >= 0 && (index & 8)) {
        rex |= X86_REX_X;
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
    if (index >= 0) {
        append_ModRM_SIB(code, mod, reg & 7, 0, index & 7, base & 7);
    } else if (base == X86_SP || base == X86_R12) {
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
 *     is64: True to prepend REX.W to an integer instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg1: Register index or sub-opcode for ModR/M reg field.
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     rtl_reg2: RTL register index from which to set ModR/M r/m field.
 */
static inline void append_insn_ModRM_ctx(
    CodeBuffer *code, bool is64, X86Opcode opcode, int reg1,
    HostX86Context *ctx, int insn_index, int rtl_reg2)
{
    if (is_spilled(ctx, insn_index, rtl_reg2)) {
        append_insn_ModRM_mem(code, is64, opcode, reg1,
                              X86_SP, -1, ctx->regs[rtl_reg2].spill_offset);
    } else {
        append_insn_ModRM_reg(code, is64, opcode, reg1,
                              ctx->regs[rtl_reg2].host_reg);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_insn_ModRM_riprel:  Append an instruction which takes a ModR/M
 * byte, encoding an EA using RIP-relative addressing.  The instruction is
 * assumed not to have any additional bytes (such as immediate data) after
 * the EA displacement.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     is64: True to prepend REX.W to an integer instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg: Register for ModR/M reg field.
 *     offset: Offset of the address to encode, counting from the base of
 *         the code buffer.
 */
static inline void append_insn_ModRM_riprel(
    CodeBuffer *code, bool is64, X86Opcode opcode, X86Register reg,
    long offset)
{
    uint8_t rex = is64 ? X86_REX_W : 0;
    if (reg & 8) {
        rex |= X86_REX_R;
    }
    if (rex) {
        append_rex_opcode(code, rex, opcode);
    } else {
        append_opcode(code, opcode);
    }
    append_ModRM(code, X86MOD_DISP0, reg & 7, X86MODRM_RIP_REL);
    /* Displacement is measured from the end of this instruction. */
    const long disp = offset - (code->len + 4);
    ASSERT((uint64_t)disp + 0x80000000 < UINT64_C(0x100000000));
    append_imm32(code, disp);
}

/*-----------------------------------------------------------------------*/

/**
 * append_vex_insn_ModRM_reg:  Append a VEX-format instruction, encoding a
 * register EA.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     vex_W: True to set VEX.W on the instruction, false otherwise.
 *     vex_L: True to set VEX.L on the instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg1: Register or sub-opcode for ModR/M reg field.
 *     reg2: Register for ModR/M r/m field.
 *     reg3: Register for VEX vvvv field, or 0 if no third register.
 */
static inline void append_vex_insn_ModRM_reg(
    CodeBuffer *code, bool vex_W, bool vex_L, X86Opcode opcode, int reg1,
    X86Register reg2, X86Register reg3)
{
    append_vex_opcode(code, opcode, vex_W, vex_L, (reg1 & 8) != 0, false,
                      (reg2 & 8) != 0, reg3 & 15);
    append_ModRM(code, X86MOD_REG, reg1 & 7, reg2 & 7);
}

/*-----------------------------------------------------------------------*/

/**
 * append_vex_insn_ModRM_mem:  Append a VEX-format instruction, encoding a
 * memory EA.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     vex_W: True to set VEX.W on the instruction, false otherwise.
 *     vex_L: True to set VEX.L on the instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg1: Register or sub-opcode for ModR/M reg field.
 *     base: Base register for memory address.
 *     index: Index register for memory address, or -1 to omit the index.
 *         Must not be X86_SP.
 *     offset: Constant offset for memory address.
 *     reg3: Register for VEX vvvv field, or 0 if no third register.
 */
static inline void append_vex_insn_ModRM_mem(
    CodeBuffer *code, bool vex_W, bool vex_L, X86Opcode opcode, int reg1,
    X86Register base, int index, int32_t offset, X86Register reg3)
{
    /* Currently, we only call this with stack references, so many of
     * these checks are meaningless. */
    const bool vex_R = (reg1 & 8) != 0;
    ASSERT(index < 0);
    const bool vex_X = false;  // index >= 0 && (index & 8) != 0
    ASSERT(base == X86_SP);
    const bool vex_B = false;  // (base & 8) != 0

    append_vex_opcode(code, opcode, vex_W, vex_L, vex_R, vex_X, vex_B,
                      reg3 & 15);

    X86Mod mod;
    if (offset == 0) {
        mod = X86MOD_DISP0;
    } else if ((uint32_t)offset + 128 < 256) {  // [-128,+127]
        mod = X86MOD_DISP8;
    } else {
        mod = X86MOD_DISP32;
    }
    append_ModRM_SIB(code, mod, reg1 & 7, 0, X86SIB_NOINDEX, X86_SP);
    if (mod == X86MOD_DISP8) {
        append_imm8(code, (uint8_t)offset);
    } else if (mod == X86MOD_DISP32) {
        append_imm32(code, (uint32_t)offset);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_vex_insn_ModRM_ctx:  Append a VEX-format instruction, encoding
 * an EA appropriate to the given source RTL register.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     vex_W: True to set VEX.W on the instruction, false otherwise.
 *     vex_L: True to set VEX.L on the instruction, false otherwise.
 *     opcode: Instruction opcode.
 *     reg1: Register or sub-opcode for ModR/M reg field.
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     rtl_reg2: RTL register index from which to set ModR/M r/m field.
 *     reg3: Register for VEX vvvv field, or 0 if no third register.
 */
static inline void append_vex_insn_ModRM_ctx(
    CodeBuffer *code, bool vex_W, bool vex_L, X86Opcode opcode, int reg1,
    HostX86Context *ctx, int insn_index, int rtl_reg2, X86Register reg3)
{
    if (is_spilled(ctx, insn_index, rtl_reg2)) {
        append_vex_insn_ModRM_mem(code, vex_W, vex_L, opcode, reg1,
                                  X86_SP, -1, ctx->regs[rtl_reg2].spill_offset,
                                  reg3);
    } else {
        append_vex_insn_ModRM_reg(code, vex_W, vex_L, opcode, reg1,
                                  ctx->regs[rtl_reg2].host_reg, reg3);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_nops:  Append no-op instructions totaling the given number of
 * bytes.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     len: Number of bytes to append.
 */
static void append_nops(CodeBuffer *code, int len)
{
    ASSERT(len >= 0);
    ASSERT(len < 16);

    switch (len) {
      case 15:
        append_opcode(code, X86OP_OPERAND_SIZE);
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM_SIB(code, X86MOD_DISP8, 0, 0, 0, 0);
        append_imm8(code, 0);
        goto nop9;
      case 14:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM_SIB(code, X86MOD_DISP8, 0, 0, 0, 0);
        append_imm8(code, 0);
        goto nop9;
      case 13:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM(code, X86MOD_DISP8, 0, 0);
        append_imm8(code, 0);
        goto nop9;
      case 12:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM(code, X86MOD_DISP0, 0, 0);
        goto nop9;
      case 11:
        append_opcode(code, X86OP_OPERAND_SIZE);
        append_opcode(code, X86OP_NOP);
        goto nop9;
      case 10:
        append_opcode(code, X86OP_NOP);
        /* fall through */
      case 9: nop9:
        append_opcode(code, X86OP_OPERAND_SIZE);
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM_SIB(code, X86MOD_DISP32, 0, 0, 0, 0);
        append_imm32(code, 0);
        break;
      case 8:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM_SIB(code, X86MOD_DISP32, 0, 0, 0, 0);
        append_imm32(code, 0);
        break;
      case 7:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM(code, X86MOD_DISP32, 0, 0);
        append_imm32(code, 0);
        break;
      case 6:
        append_opcode(code, X86OP_OPERAND_SIZE);
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM_SIB(code, X86MOD_DISP8, 0, 0, 0, 0);
        append_imm8(code, 0);
        break;
      case 5:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM_SIB(code, X86MOD_DISP8, 0, 0, 0, 0);
        append_imm8(code, 0);
        break;
      case 4:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM(code, X86MOD_DISP8, 0, 0);
        append_imm8(code, 0);
        break;
      case 3:
        append_opcode(code, X86OP_NOP_Ev);
        append_ModRM(code, X86MOD_DISP0, 0, 0);
        break;
      case 2:
        append_opcode(code, X86OP_OPERAND_SIZE);
        append_opcode(code, X86OP_NOP);
        break;
      case 1:
        append_opcode(code, X86OP_NOP);
        break;
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
static inline void append_move(CodeBuffer *code, RTLDataType type,
                               X86Register host_dest, X86Register host_src)
{
    switch (type) {
      case RTLTYPE_INT32:
      case RTLTYPE_FPSTATE:
        ASSERT(host_dest <= X86_R15);
        ASSERT(host_src <= X86_R15);
        append_insn_ModRM_reg(code, false, X86OP_MOV_Gv_Ev,
                              host_dest, host_src);
        return;
      case RTLTYPE_INT64:
      case RTLTYPE_ADDRESS:
        ASSERT(host_dest <= X86_R15);
        ASSERT(host_src <= X86_R15);
        append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                              host_dest, host_src);
        return;
      default:
        ASSERT(!rtl_type_is_int(type));
        ASSERT(host_dest >= X86_XMM0);
        ASSERT(host_src >= X86_XMM0);
        /* The Intel optimization guidelines state: (1) avoid mixed use of
         * integer/FP operations on the same register (thus MOVAPS instead
         * of MOVDQA); (2) use PS instead of PD if both operations are
         * bitwise-equivalent (thus MOVAPS even for double-precision types). */
        append_insn_ModRM_reg(code, false, X86OP_MOVAPS_V_W,
                              host_dest, host_src);
        return;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_move_gpr:  Append an instruction to copy (MOV) one integer
 * register to another.
 *
 * Specialization of append_move() for GPRs.
 */
static inline void append_move_gpr(CodeBuffer *code, RTLDataType type,
                                   X86Register host_dest, X86Register host_src)
{
    ASSERT(rtl_type_is_int(type) || type == RTLTYPE_FPSTATE);
    append_insn_ModRM_reg(code, int_type_is_64(type), X86OP_MOV_Gv_Ev,
                          host_dest, host_src);
}

/*-----------------------------------------------------------------------*/

/**
 * append_load_imm_gpr:  Append an instruction to load an immediate value
 * into an integer register.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     host_dest: Destination register.
 *     imm: Immediate value to load.
 */
static inline void append_load_imm_gpr(CodeBuffer *code,
                                       X86Register host_dest, uint64_t imm)
{
    if (imm <= UINT64_C(0xFFFFFFFF)) {
        append_insn_R(code, false, X86OP_MOV_rAX_Iv, host_dest);
        append_imm32(code, (uint32_t)imm);
    } else if (imm >= UINT64_C(0xFFFFFFFF80000000)) {
        append_insn_ModRM_reg(code, true, X86OP_MOV_Ev_Iz, 0, host_dest);
        append_imm32(code, (uint32_t)imm);
    } else {
        append_insn_R(code, true, X86OP_MOV_rAX_Iv, host_dest);
        append_imm64(code, imm);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_load:  Append an instruction to load a register from a memory
 * location.  The memory address is assumed to be properly aligned.
 *
 * V2_FLOAT32 operands load a full XMM register (16 bytes).  To load only
 * the 8 bytes of actual data, use the FLOAT64 type.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     type: Register type (RTLTYPE_*).
 *     host_dest: Destination host register.
 *     host_base: Host register for memory address base.
 *     host_index: Host register for memory address index, or -1 if no index.
 *     offset: Access offset from base register.
 */
static inline void append_load(
    CodeBuffer *code, RTLDataType type, X86Register host_dest,
    X86Register host_base, int host_index, int32_t offset)
{
    switch (type) {
      case RTLTYPE_INT32:
      case RTLTYPE_FPSTATE:
        append_insn_ModRM_mem(code, false, X86OP_MOV_Gv_Ev,
                              host_dest, host_base, host_index, offset);
        return;
      case RTLTYPE_INT64:
      case RTLTYPE_ADDRESS:
        append_insn_ModRM_mem(code, true, X86OP_MOV_Gv_Ev,
                              host_dest, host_base, host_index, offset);
        return;
      case RTLTYPE_FLOAT32:
        append_insn_ModRM_mem(code, false, X86OP_MOVSS_V_W,
                              host_dest, host_base, host_index, offset);
        return;
      case RTLTYPE_FLOAT64:
        append_insn_ModRM_mem(code, false, X86OP_MOVSD_V_W,
                              host_dest, host_base, host_index, offset);
        return;
      default:
        ASSERT(rtl_type_is_vector(type));
        append_insn_ModRM_mem(code, false, X86OP_MOVAPS_V_W,
                              host_dest, host_base, host_index, offset);
        return;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_load_gpr:  Append an instruction to load an integer register
 * from a memory location.  The memory address is assumed to be properly
 * aligned.
 *
 * Specialization of append_load() for GPRs and host_index == -1.
 */
static inline void append_load_gpr(
    CodeBuffer *code, RTLDataType type, X86Register host_dest,
    X86Register host_base, int32_t offset)
{
    ASSERT(rtl_type_is_int(type) || type == RTLTYPE_FPSTATE);
    append_insn_ModRM_mem(code, int_type_is_64(type), X86OP_MOV_Gv_Ev,
                          host_dest, host_base, -1, offset);
}

/*-----------------------------------------------------------------------*/

/**
 * append_store:  Append an instruction to store a register to a memory
 * location.  The memory address is assumed to be properly aligned.
 *
 * V2_FLOAT32 operands store a full XMM register (16 bytes).  To store
 * only the 8 bytes of actual data, use the FLOAT64 type.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     type: Register type (RTLTYPE_*).
 *     host_src: Destination host register.
 *     host_base: Host register for memory address base.
 *     host_index: Host register for memory address index, or -1 if no index.
 *     offset: Access offset from base register.
 */
static inline void append_store(
    CodeBuffer *code, RTLDataType type, X86Register host_src,
    X86Register host_base, int host_index, int32_t offset)
{
    switch (type) {
      case RTLTYPE_INT32:
      case RTLTYPE_FPSTATE:
        append_insn_ModRM_mem(code, false, X86OP_MOV_Ev_Gv, host_src,
                              host_base, host_index, offset);
        return;
      case RTLTYPE_INT64:
      case RTLTYPE_ADDRESS:
        append_insn_ModRM_mem(code, true, X86OP_MOV_Ev_Gv, host_src,
                              host_base, host_index, offset);
        return;
      case RTLTYPE_FLOAT32:
        append_insn_ModRM_mem(code, false, X86OP_MOVSS_W_V, host_src,
                              host_base, host_index, offset);
        return;
      case RTLTYPE_FLOAT64:
        append_insn_ModRM_mem(code, false, X86OP_MOVSD_W_V, host_src,
                              host_base, host_index, offset);
        return;
      default:
        ASSERT(rtl_type_is_vector(type));
        append_insn_ModRM_mem(code, false, X86OP_MOVAPS_W_V, host_src,
                              host_base, host_index, offset);
        return;
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
static inline void append_load_alias(
    CodeBuffer *code, const HostX86Context *ctx, const RTLAlias *alias,
    X86Register host_dest)
{
    const X86Register host_base =
        alias->base ? ctx->regs[alias->base].host_reg : X86_SP;
    append_load(code, alias->type, host_dest, host_base, -1, alias->offset);
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
static inline void append_store_alias(
    CodeBuffer *code, const HostX86Context *ctx, const RTLAlias *alias,
    X86Register host_src)
{
    if (alias->base) {
        /* We store V2_FLOAT32 as a full XMM register on the stack, but we
         * need to store only the 8 bytes with actual data if writing to
         * bound storage. */
        const RTLDataType store_type = ((alias->type == RTLTYPE_V2_FLOAT32)
                                        ? RTLTYPE_FLOAT64 : alias->type);
        append_store(code, store_type, host_src,
                     ctx->regs[alias->base].host_reg, -1, alias->offset);
    } else {
        append_store(code, alias->type, host_src, X86_SP, -1, alias->offset);
    }
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
static inline void append_move_or_load(
    CodeBuffer *code, const HostX86Context *ctx, const RTLUnit *unit,
    int insn_index, int host_dest, int src)
{
    if (is_spilled(ctx, insn_index, src)) {
        append_load(code, unit->regs[src].type, host_dest,
                    X86_SP, -1, ctx->regs[src].spill_offset);
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
 * Specialization of append_move_or_load() for GPRs.  As a special case
 * (used by translate_call()), if the register is not live at all, it is
 * assumed to be a constant which can be loaded directly into the target
 * register.
 */
static inline void append_move_or_load_gpr(
    CodeBuffer *code, const HostX86Context *ctx, const RTLUnit *unit,
    int insn_index, int host_dest, int src)
{
    if (!unit->regs[src].live) {
        ASSERT(unit->regs[src].source == RTLREG_CONSTANT);
        ASSERT(rtl_register_is_int(&unit->regs[src]));
        append_load_imm_gpr(code, host_dest, unit->regs[src].value.i64);
    } else if (is_spilled(ctx, insn_index, src)) {
        append_load_gpr(code, unit->regs[src].type, host_dest,
                        X86_SP, ctx->regs[src].spill_offset);
    } else if (ctx->regs[src].host_reg != host_dest) {
        append_move_gpr(code, unit->regs[src].type, host_dest,
                        ctx->regs[src].host_reg);
    }
}

/*-----------------------------------------------------------------------*/

/**
 * append_compare:  Append an appropriate comparison instruction for the
 * given parameters.  src2==0 implies a register-immediate compare.
 *
 * If the operation is a floating-point compare, it must be a comparison
 * that only requires one test (GT or GE).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     code: Output code buffer.
 *     src1: RTL register containing first comparand.
 *     src2: RTL register containing second comparand, or 0 for a
 *         register-immediate compare.
 *     src_imm: Immediate comparand if src2 == 0.
 *     src1_temp: Temporary host register for reloading src1.  Ignored if
 *         src1 does not need to be reloaded.
 *     icmp_eq: True if an integer comparison is an equality comparison.
 *         Used to eliminate comparisons against immediate 0 after an
 *         instruction which does not set the full set of flags.
 *     fcmp_ordered: True if a floating-point comparison should use the
 *         ordered comparison instruction (COMIS[SD]), false otherwise.
 *         Ignored for integer compares.
 *     clear_reg: X86Register to clear before performing the comparison,
 *         or -1 to not clear any register.
 * [Return value]
 *     True if a comparison instruction was added; false if the comparison
 *     was optimized out.
 */
static bool append_compare(
    HostX86Context *ctx, int insn_index, CodeBuffer *code, int src1,
    int src2, int32_t src_imm, X86Register src1_temp, bool icmp_eq,
    bool fcmp_ordered, int clear_reg)
{
    const RTLUnit * const unit = ctx->unit;
    const RTLRegister * const src1_reg = &unit->regs[src1];
    const HostX86RegInfo * const src1_info = &ctx->regs[src1];
    X86Register host_src1 = src1_info->host_reg;

    if (src2) {
        if (ctx->last_cmp_reg == src1 && ctx->last_cmp_target == src2) {
            return false;
        }
        if (is_spilled(ctx, insn_index, src1)) {
            host_src1 = src1_temp;
            append_load(code, src1_reg->type, host_src1,
                        X86_SP, -1, src1_info->spill_offset);
        }
        if (clear_reg >= 0) {
            append_insn_ModRM_reg(code, false, X86OP_XOR_Gv_Ev,
                                  clear_reg, clear_reg);
        }
        if (rtl_register_is_int(src1_reg)) {
            const bool is64 = int_type_is_64(src1_reg->type);
            append_insn_ModRM_ctx(code, is64, X86OP_CMP_Gv_Ev, host_src1,
                                  ctx, insn_index, src2);
        } else {
            const bool is64 = (src1_reg->type == RTLTYPE_FLOAT64);
            const X86Opcode opcode = is64
                ? (fcmp_ordered ? X86OP_COMISD : X86OP_UCOMISD)
                : (fcmp_ordered ? X86OP_COMISS : X86OP_UCOMISS);
            append_insn_ModRM_ctx(code, false, opcode, host_src1,
                                  ctx, insn_index, src2);
        }
        if (ctx->handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = src1;
            ctx->last_cmp_target = src2;
        }

    } else if (src_imm) {
        if (ctx->last_cmp_reg == src1
         && ctx->last_cmp_target == 0
         && ctx->last_cmp_imm == src_imm) {
            return false;
        }
        if (clear_reg >= 0) {
            append_insn_ModRM_reg(code, false, X86OP_XOR_Gv_Ev,
                                  clear_reg, clear_reg);
        }
        ASSERT(rtl_register_is_int(src1_reg));
        const bool is64 = int_type_is_64(src1_reg->type);
        if ((uint32_t)src_imm + 128 < 256) {
            append_insn_ModRM_ctx(code, is64, X86OP_IMM_Ev_Ib, X86OP_IMM_CMP,
                                  ctx, insn_index, src1);
            append_imm8(code, (uint8_t)src_imm);
        } else {
            append_insn_ModRM_ctx(code, is64, X86OP_IMM_Ev_Iz, X86OP_IMM_CMP,
                                  ctx, insn_index, src1);
            append_imm32(code, src_imm);
        }
        if (ctx->handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = src1;
            ctx->last_cmp_target = 0;
            ctx->last_cmp_imm = src_imm;
        }

    } else {
        if (icmp_eq && ctx->last_test_reg == src1) {
            return false;
        }
        if (ctx->last_cmp_reg == src1
         && ctx->last_cmp_target == 0
         && ctx->last_cmp_imm == 0) {
            return false;
        }
        if (clear_reg >= 0) {
            append_insn_ModRM_reg(code, false, X86OP_XOR_Gv_Ev,
                                  clear_reg, clear_reg);
        }
        ASSERT(rtl_register_is_int(src1_reg));
        const bool is64 = int_type_is_64(src1_reg->type);
        if (is_spilled(ctx, insn_index, src1)) {
            append_insn_ModRM_mem(code, is64, X86OP_IMM_Ev_Ib, X86OP_IMM_CMP,
                                  X86_SP, -1, src1_info->spill_offset);
            append_imm8(code, 0);
        } else {
            append_insn_ModRM_reg(code, is64, X86OP_TEST_Ev_Gv,
                                  host_src1, host_src1);
        }
        if (ctx->handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
            ctx->last_test_reg = src1;
            ctx->last_cmp_reg = src1;
            ctx->last_cmp_target = 0;
            ctx->last_cmp_imm = 0;
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * append_jump_raw:  Append a JMP or Jcc instruction with the given
 * displacement.  The displacement is encoded in 8 bits if in the range
 * [-127,+128] and in 32 bits otherwise; the opcode is assumed to be
 * correct for the displacement size.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     opcode: Jump opcode.
 *     disp: Displacement to encode.
 */
static inline void append_jump_raw(CodeBuffer *code, X86Opcode opcode,
                                   int32_t disp)
{
    append_opcode(code, opcode);
    if (((uint32_t)disp + 128) < 256) {  // i.e., disp is in [-128,+127]
        append_imm8(code, (uint8_t)disp);
    } else {
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
static void append_jump(
    CodeBuffer *code, HostX86BlockInfo *block_info,
    X86Opcode short_opcode, X86Opcode long_opcode, int label, long target)
{
    if (target >= 0) {
        const int64_t offset = target - code->len;
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

/*-----------------------------------------------------------------------*/

/**
 * reload_base_and_index:  Return the host registers containing the values
 * of the base and (if present) index registers for the given memory access
 * instruction, reloading spilled registers if necessary.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     fallback: Register to use as a fallback for reloads.
 *     base_ret: Pointer to variable to receive the base register
 *         (X86Register).
 *     index_ret: Pointer to variable to receive the index register
 *         (X86Register), or -1 if there is no index register for the access.
 */
static void reload_base_and_index(
    CodeBuffer *code, const HostX86Context *ctx, int insn_index,
    X86Register fallback, X86Register *base_ret, int *index_ret)
{
    const RTLUnit * const unit = ctx->unit;
    const RTLInsn * const insn = &unit->insns[insn_index];
    int base = insn->src1;
    int index = insn->host_data_16;

    /* For indexed accesses, if base is spilled but index is not, swap the
     * two registers.  This ensures that if the fallback register overlaps
     * the index register (such as in a load operation when the destination
     * is the same as the index register), we don't clobber the index when
     * we reload the base. */
    if (index && is_spilled(ctx, insn_index, base)
              && !is_spilled(ctx, insn_index, index)) {
        const int temp = base;
        base = index;
        index = temp;
    }

    /* Save the current output position in case we end up needing to load
     * the index first (for the 64+32 add case below). */
    const long base_reload_pos = code->len;

    if (!is_spilled(ctx, insn_index, base)) {
        *base_ret = ctx->regs[base].host_reg;
    } else {
        /* This could be INT32/INT64 if address operand optimization
         * eliminated a ZCAST. */
        ASSERT(rtl_register_is_int(&unit->regs[base]));
        append_load(code, unit->regs[base].type, fallback,
                    X86_SP, -1, ctx->regs[base].spill_offset);
        *base_ret = fallback;
    }

    if (index) {
        if (!is_spilled(ctx, insn_index, index)) {
            *index_ret = ctx->regs[index].host_reg;
        } else {
            ASSERT(rtl_register_is_int(&unit->regs[index]));
            if (*base_ret == fallback) {
                /* We should always have a separate temporary if we have to
                 * reload a spilled index. */
                ASSERT(is_spilled(ctx, insn_index, base));
                if (int_type_is_64(unit->regs[index].type)) {
                    append_insn_ModRM_mem(
                        code, true, X86OP_ADD_Gv_Ev, fallback,
                        X86_SP, -1, ctx->regs[index].spill_offset);
                } else if (int_type_is_64(unit->regs[base].type)) {
                    /* The base register is 64 bits, so we can load the
                     * index first and add the base from its spill slot. */
                    code->len = base_reload_pos;
                    append_load(code, RTLTYPE_INT32, fallback,
                                X86_SP, -1, ctx->regs[index].spill_offset);
                    append_insn_ModRM_mem(
                        code, true, X86OP_ADD_Gv_Ev, fallback,
                        X86_SP, -1, ctx->regs[base].spill_offset);
                } else {
                    /* This is tricky: we have to add two 32-bit values
                     * from memory and get a 64-bit sum without using any
                     * other registers or memory.  Since both values are
                     * 32 bits wide, there'll be at most a carry of 1 into
                     * the high word, so we do the addition in 32 bits and
                     * handle the carry manually.  Fortunately, this case
                     * should be extremely rare in practice. */
                    log_warning(ctx->handle, "Slow add of spilled 32-bit"
                                    " base and index at %d", insn_index);
                    append_insn_ModRM_mem(
                        code, false, X86OP_ADD_Gv_Ev, fallback,  // 32-bit add!
                        X86_SP, -1, ctx->regs[index].spill_offset);
                    const long jump_pos = code->len;
                    append_jump_raw(code, X86OP_JNC_Jb, 0);
                    const long jump_end = code->len;
                    ASSERT(jump_end == jump_pos + 2);
                    append_insn_ModRM_reg(code, true, X86OP_SHIFT_Ev_Ib,
                                          X86OP_SHIFT_ROR, fallback);
                    append_imm8(code, 32);
                    append_insn_ModRM_reg(code, true, X86OP_IMM_Ev_Ib,
                                          X86OP_IMM_ADD, fallback);
                    append_imm8(code, 1);
                    append_insn_ModRM_reg(code, true, X86OP_SHIFT_Ev_Ib,
                                          X86OP_SHIFT_ROR, fallback);
                    append_imm8(code, 32);
                    ASSERT(code->len - jump_end <= 127);
                    code->buffer[jump_end-1] = (uint8_t)(code->len - jump_end);
                }
                *index_ret = -1;
            } else {
                append_load(code, unit->regs[index].type, fallback,
                            X86_SP, -1, ctx->regs[index].spill_offset);
                *index_ret = fallback;
            }
        }
    } else {
        *index_ret = -1;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * reload_store_source_gpr:  Return the GPR containing the data value for
 * an integer or byte-reversed float store instruction.  If necessary,
 * save RAX in XMM15; the caller needs to restore it after the store in
 * this case.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     ctx: Translation context.
 *     insn_index: Index of current instruction in ctx->unit->insns[].
 *     host_base_ptr: Pointer to Base register for address (X86Register).
 *         May be modified on return.
 *     host_index_ptr: Pointer to index register for address (X86Register,
 *         -1 if none).  May be modified on return.
 *     host_value_ret: Pointer to variable to receive the value register
 *         (X86Register).
 * [Return value]
 *     True if RAX was saved to XMM15, false otherwise.
 */
static bool reload_store_source_gpr(
    CodeBuffer *code, const HostX86Context *ctx, int insn_index,
    X86Register *host_base_ptr, int *host_index_ptr,
    X86Register *host_value_ret)
{
    const RTLUnit * const unit = ctx->unit;
    const RTLInsn * const insn = &unit->insns[insn_index];
    const int src2 = insn->src2;
    const RTLRegister * const src2_reg = &unit->regs[src2];
    const HostX86RegInfo * const src2_info = &ctx->regs[src2];

    RTLDataType type = src2_reg->type;
    bool is_float = false;
    if (type == RTLTYPE_FLOAT32) {
        is_float = true;
        type = RTLTYPE_INT32;
    } else if (type == RTLTYPE_FLOAT64) {
        is_float = true;
        type = RTLTYPE_INT64;
    }
    const bool is64 = int_type_is_64(type);

    const bool spilled = is_spilled(ctx, insn_index, src2);
    if (!is_float && !spilled) {
        /* If the value to be stored is the same as the base or index
         * register and MOVBE is not in use, we have to use a temporary for
         * the byte-swapped value (see notes in allocate_regs_for_insn()). */
        bool bswap_src2_collision = false;
        if (!(ctx->handle->setup.host_features & BINREC_FEATURE_X86_MOVBE)) {
            if (insn->opcode == RTLOP_STORE_BR
             || insn->opcode == RTLOP_STORE_I16_BR) {
                bswap_src2_collision =
                    (ctx->regs[src2].host_reg == *host_base_ptr
                     || ctx->regs[src2].host_reg == *host_index_ptr);
            }
        }
        if (!bswap_src2_collision) {
            *host_value_ret = src2_info->host_reg;
            return false;
        }
    }

    /* insn->src3 is not an RTL register here!  Instead it holds a
     * temporary X86Register for reloading a spilled store source value.
     * See the allocation logic at the top of allocate_regs_for_insn()
     * for details. */
    X86Register host_value = (X86Register)insn->src3;
    if (host_value != *host_base_ptr && (int)host_value != *host_index_ptr) {
        if (spilled) {
            append_load_gpr(code, type, host_value,
                            X86_SP, src2_info->spill_offset);
        } else if (is_float) {
            append_insn_ModRM_reg(code, is64, X86OP_MOVD_E_V,
                                  src2_info->host_reg, host_value);
        } else {
            append_move_gpr(code, type, host_value, src2_info->host_reg);
        }
        *host_value_ret = host_value;
        return false;
    }

    ASSERT(host_value == X86_R15);
    /* If we get here, base or index is in R15 due to a spill reload.
     * For an indexed access, if one of the two registers is unspilled, we
     * always use that register as the base (see reload_base_and_index()),
     * so host_index will be R15.  If both registers are spilled or it's
     * not an indexed access, host_index will be -1.  So we only need to
     * check host_base for RAX collision here. */
    ASSERT(*host_index_ptr != X86_AX);
    if (*host_base_ptr == X86_AX) {
        ASSERT(*host_index_ptr == X86_R15);
        append_insn_ModRM_reg(code, true, X86OP_ADD_Gv_Ev, X86_R15, X86_AX);
        *host_base_ptr = X86_R15;
        *host_index_ptr = -1;
    }
    append_insn_ModRM_reg(code, true, X86OP_MOVD_V_E, X86_XMM15, X86_AX);
    if (spilled) {
        append_load_gpr(code, type, X86_AX, X86_SP, src2_info->spill_offset);
    } else if (is_float) {
        append_insn_ModRM_reg(code, is64, X86OP_MOVD_E_V,
                              src2_info->host_reg, X86_AX);
    } else {
        append_move_gpr(code, type, X86_AX, src2_info->host_reg);
    }
    *host_value_ret = X86_AX;
    return true;
}

/*************************************************************************/
/************************* Alias/spill handling **************************/
/*************************************************************************/

/* Maximum length of alias setup code generated by reload_regs_for_block().
 * If the input code buffer has at least this much space available, it will
 * not be expanded.  The worst case is:
 *    - 14 GPR spills with REX prefixes = 112 bytes
 *    - 14 GPR loads with REX prefixes = 112 bytes
 *    - 14 XMM exchanges with REX prefixes = 168 bytes
 *    - 1 XMM load with a REX prefix = 9 bytes
 * for a total of 401 bytes.  We round up to 416 to potentially help the
 * compiler optimize when using a temporary buffer. */
#define RELOAD_REGS_SIZE  416

/**
 * reload_regs_for_block:  Reload host registers with the values expected
 * on entry to the given block.  Merged alias values are moved or reloaded
 * to their designated merge registers, and if the control flow edge being
 * traversed is a backward branch, reload any registers whose spills are
 * crossed by the branch.
 *
 * The code buffer passed to this function may be a temporary buffer of
 * size RELOAD_REGS_SIZE or greater; in that case, the function will
 * always succeed.
 *
 * [Parameters]
 *     code: Output code buffer (may be a temporary buffer).
 *     ctx: Translation context.
 *     block_index: Index of current basic block in ctx->unit->blocks[].
 *     target_block: Index of target basic block in ctx->unit->blocks[].
 * [Return value]
 *     True on success, false if out of memory.
 */
static bool reload_regs_for_block(
    CodeBuffer *code, HostX86Context *ctx, int block_index, int target_block)
{
    RTLUnit * const unit = ctx->unit;
    const int num_aliases = unit->next_alias;
    const int last_insn = unit->blocks[block_index].last_insn;
    const int target_insn = unit->blocks[target_block].first_insn;
    const uint16_t *current_store = ctx->blocks[block_index].alias_store;
    const uint16_t *next_load = ctx->blocks[target_block].alias_load;

    if (UNLIKELY(code->buffer_size - code->len < RELOAD_REGS_SIZE)) {
        ASSERT(code->buffer == ctx->handle->code_buffer);
        ctx->handle->code_len = code->len;
        if (UNLIKELY(!binrec_ensure_code_space(ctx->handle,
                                               RELOAD_REGS_SIZE))) {
            log_error(ctx->handle, "No memory for register reload for block"
                      " %d->%d", block_index, target_block);
            return false;
        }
        code->buffer = ctx->handle->code_buffer;
        code->buffer_size = ctx->handle->code_buffer_size;
    }

    /* If this is a forward branch, first spill any registers whose spills
     * are crossed by the branch. */
    if (unit->blocks[block_index].next_block >= 0
     && target_block > unit->blocks[block_index].next_block) {
        for (int i = 0; i < 32; i++) {
            /* We don't need to call current_reg() here since we check
             * reg->death separately below. */
            const int reg_index = ctx->reg_map[i];
            if (reg_index) {
                const RTLRegister *reg = &unit->regs[reg_index];
                if (reg->death >= target_insn
                 && is_spilled(ctx, target_insn, reg_index)) {
                    /* The register can't be spilled if it's in the live
                     * map (since it would have been overwritten by the
                     * register that spilled it). */
                    ASSERT(!is_spilled(ctx, last_insn, reg_index));
                    const HostX86RegInfo *reg_info = &ctx->regs[reg_index];
                    append_store(code, reg->type, reg_info->host_reg,
                                 X86_SP, -1, reg_info->spill_offset);
                }
            }
        }
    }

    /* Construct a map of which registers get moved where and which need
     * to be reloaded from spill slots or loaded from alias storage.  We
     * need a separate step for this in case the target of one move would
     * overwrite the source of another, in which case we have to use a swap
     * instead.  This can get particularly tricky if several registers are
     * shifted in a loop (see tests/host-x86/general/alias-merge-swap-cycle-*
     * for some examples). */
    uint32_t move_targets = 0;    // Bit set = register is a move target
    uint32_t reload_targets = 0;  // Bit set = register is a reload target
    uint32_t load_targets = 0;    // Bit set = register is a load target
    uint8_t move_map[32];  // X86Register source for each move target
    uint8_t src_map[32];   // Map from original to current (swapped) registers
                           //    (i.e., "where is this register's value now?")
    uint8_t value_map[32]; // Map from original reg values to current locations
                           //    (i.e., "what does this register now hold?")
    uint8_t src_count[32]; // # of times each host register is used as a source
                           //    (indexed by original host register)
    uint8_t dest_type[32]; // RTLDataType of each alias, indexed by move target
    uint16_t reload_map[32]; // RTL register to load into each reload target
    uint16_t load_map[32]; // RTL alias to load into each load target
    memset(src_count, 0, sizeof(src_count));

    for (int i = 1; i < num_aliases; i++) {
        const int merge_reg = next_load[i];
        if (merge_reg && ctx->regs[merge_reg].merge_alias) {
            const X86Register host_dest = ctx->regs[merge_reg].host_merge;
            dest_type[host_dest] = unit->regs[merge_reg].type;
            value_map[host_dest] = host_dest;
            const int store_reg = current_store[i];
            if (store_reg) {
                if (is_spilled(ctx, last_insn, store_reg)) {
                    reload_targets |= 1 << host_dest;
                    reload_map[host_dest] = (uint16_t)store_reg;
                } else {
                    const X86Register host_src = ctx->regs[store_reg].host_reg;
                    move_targets |= 1 << host_dest;
                    move_map[host_dest] = host_src;
                    src_map[host_src] = host_src;
                    value_map[host_src] = host_src;
                    src_count[host_src]++;
                }
            } else {
                load_targets |= 1 << host_dest;
                load_map[host_dest] = (uint16_t)i;
            }
        }
    }

    /* If this is a backward branch, also include reloads of registers
     * which are spilled now but were not spilled at the branch target. */
    if (unit->blocks[block_index].next_block < 0
     || target_block < unit->blocks[block_index].next_block) {
        const HostX86BlockInfo *target_info = &ctx->blocks[target_block];
        for (int i = 0; i < 32; i++) {
            const int reg_index = target_info->initial_reg_map[i];
            if (reg_index) {
                /* If the register is live on entry to the target block,
                 * it can't have been chosen as a merge target, since merge
                 * targets are always GET_ALIAS outputs (which by SSA are
                 * not live before the GET_ALIAS instruction). */
                ASSERT(!(move_targets & (1 << i)));
                ASSERT(!(reload_targets & (1 << i)));
                ASSERT(!(load_targets & (1 << i)));
                const RTLRegister *reg = &unit->regs[reg_index];
                /* The register's live range should have been extended to
                 * the last backward branch that targets a block where
                 * it's live. */
                ASSERT(reg->death >= last_insn);
                if (is_spilled(ctx, last_insn, reg_index)) {
                    reload_targets |= 1 << i;
                    reload_map[i] = reg_index;
                }
            }
        }
    }

    /* Add any registers which are live and unspilled both here and at the
     * beginning of the target block as moves to themselves, to ensure that
     * their values don't get lost during register shuffling. */
    for (int i = 0; i < 32; i++) {
        const int reg = ctx->reg_map[i];
        const int later_insn = max(last_insn, target_insn);
        if (reg
         && (target_insn > last_insn || unit->regs[reg].birth < target_insn)
         && unit->regs[reg].death >= later_insn
         && !is_spilled(ctx, later_insn, reg)) {
            ASSERT(!(move_targets & (1 << i)));
            move_targets |= 1 << i;
            move_map[i] = i;
            src_map[i] = i;
            value_map[i] = i;
            src_count[i]++;
        }
    }

    /* Now actually perform the moves, swapping registers as needed.  In
     * order to avoid clobbering values in certain move patterns (see
     * tests/host-x86/general/alias-merge-source-live-no-swap.c for an
     * example), we initially only move registers whose targets are not
     * themselves used as sources.  If no moves can be done during a pass
     * but there are still registers left to be moved, then the remaining
     * moves must form one or more cycles, so we swap one pair to try and
     * break a cycle, then start a new pass. */
    while (move_targets) {
        uint32_t move_targets_pass = move_targets;
        bool resolved_any = false;
        while (move_targets_pass) {
            const X86Register host_dest = ctz32(move_targets_pass);
            move_targets_pass ^= 1 << host_dest;
            const X86Register move_src = move_map[host_dest];
            const X86Register host_src = src_map[move_src];
            if (host_src != host_dest) {
                if (src_count[value_map[host_dest]] > 0) {
                    /* The value in the register we're about to write is
                     * still needed, so skip it for now. */
                    continue;
                }
                append_move(code, dest_type[host_dest], host_dest, host_src);
            }
            move_targets ^= 1 << host_dest;  // Register was resolved.
            resolved_any = true;
            src_count[move_src]--;
        }
        if (move_targets && !resolved_any) {
            /* There's a cycle in the move graph, so pick the first target
             * remaining and swap it with its source value. */
            const X86Register host_dest = ctz32(move_targets);
            move_targets ^= 1 << host_dest;
            const X86Register move_src = move_map[host_dest];
            const X86Register host_src = src_map[move_src];
            ASSERT(host_src != host_dest);  // Or it would already be resolved.
            /* Swap the registers, then update maps so we know where the
             * values have gone.  There's no equivalent of the XCHG
             * instruction for XMM registers, so use the XOR method
             * (a^=b, b^=a, a^=b) in that case. */
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
                /* Slight laziness here: always exchange 64 bits even if
                 * the values in both registers are only 32 bits wide. */
                append_insn_ModRM_reg(code, true, X86OP_XCHG_Ev_Gv,
                                      host_src, host_dest);
            }
            value_map[host_src] = value_map[host_dest];
            src_map[value_map[host_dest]] = host_src;
            value_map[host_dest] = move_src;
            src_map[move_src] = host_dest;
        }
    }

    /* Finally, load values from storage which were spilled or not live. */
    while (reload_targets) {
        const X86Register host_dest = ctz32(reload_targets);
        reload_targets ^= 1 << host_dest;
        const int src = reload_map[host_dest];
        append_load(code, unit->regs[src].type, host_dest,
                    X86_SP, -1, ctx->regs[src].spill_offset);
    }
    while (load_targets) {
        const X86Register host_dest = ctz32(load_targets);
        load_targets ^= 1 << host_dest;
        append_load_alias(code, ctx, &unit->aliases[load_map[host_dest]],
                          host_dest);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * check_reload_conflicts:  Return whether any alias or spill reloads
 * required for the branch at branch_insn collide with any live registers.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of current basic block in ctx->unit->blocks[].
 *     branch_insn: Index of branch instruction in ctx->unit->insns[].
 * [Return value]
 *     True if any spill reloads would collide with a live register,
 *     false otherwise.
 */
static bool check_reload_conflicts(const HostX86Context *ctx, int block_index,
                                   int branch_insn)
{
    ASSERT(branch_insn == ctx->unit->blocks[block_index].last_insn);

    RTLUnit * const unit = ctx->unit;
    const int target_label = unit->insns[branch_insn].label;
    const int target_block = unit->label_blockmap[target_label];
    const int target_insn = unit->blocks[target_block].first_insn;

    /* Check for alias reload conflicts.  A conflict occurs when the
     * destination of a register move or reload is either:
     *    - live past the branch (including registers which are dead at
     *      the branch but alias-merged into the next block) -- this set
     *      of registers is recorded in the block's end_live field; or
     *    - dead at the branch, but alias-merged into the target block --
     *      this covers cases where the reload would generate an XCHG
     *      instruction (or an equivalent operation for XMM registers),
     *      leaving the registers swapped if the branch was not taken.
     * Make sure to cover both cases here. */
    const int num_aliases = unit->next_alias;
    const uint16_t *current_store = ctx->blocks[block_index].alias_store;
    const uint16_t *next_load = ctx->blocks[target_block].alias_load;
    uint32_t conflict_regs = ctx->blocks[block_index].end_live;
    for (int i = 1; i < num_aliases; i++) {
        const int merge_reg = next_load[i];
        if (merge_reg && ctx->regs[merge_reg].merge_alias) {
            const int merge_src = current_store[i];
            if (!is_spilled(ctx, branch_insn, merge_src)) {
                conflict_regs |= 1 << ctx->regs[merge_src].host_reg;
            }
        }
    }
    for (int i = 1; i < num_aliases; i++) {
        const int merge_reg = next_load[i];
        if (merge_reg && ctx->regs[merge_reg].merge_alias) {
            const int merge_src = current_store[i];
            const X86Register host_src = ctx->regs[merge_src].host_reg;
            const X86Register host_dest = ctx->regs[merge_reg].host_merge;
            const bool move_required =
                (!merge_src || is_spilled(ctx, branch_insn, merge_src)
                 || host_src != host_dest);
            if (move_required && (conflict_regs & (1 << host_dest))) {
                return true;
            }
        }
    }

    /* If this is a backward branch, also check for spill reload conflicts. */
    if (target_insn < branch_insn) {
        const uint16_t *current_map = ctx->reg_map;
        const uint16_t *next_map = ctx->blocks[target_block].initial_reg_map;
        uint32_t live = ctx->blocks[block_index].end_live;
        while (live) {
            const int host_reg = ctz32(live);
            live ^= 1 << host_reg;
            if (next_map[host_reg]
             && current_map[host_reg] != next_map[host_reg]) {
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
        (handle->setup.host == BINREC_ARCH_X86_64_WINDOWS_SEH);

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
    total_stack_use += ctx->frame_size + ctx->frame_callee_reserve;
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
        const int size = 4 + code_size;
        ASSERT(size <= (int)sizeof(unwind_info));
        memmove(unwind_info + 4, unwind_info + unwind_pos, code_size);
        unwind_info[1] = prologue_pos;
        unwind_info[2] = code_size / 2;

        const int alignment = handle->code_alignment;
        ASSERT(alignment >= 8);
        int code_offset = 8 + size;
        if (code_offset % alignment != 0) {
            code_offset += alignment - (code_offset % alignment);
        }
        if (UNLIKELY(!binrec_ensure_code_space(handle, code_offset))) {
            log_error(handle, "No memory for Windows SEH data");
            return false;
        }

        uint8_t *buffer = handle->code_buffer;
        *ALIGNED_CAST(uint64_t *, buffer) = bswap_le64(code_offset);
        memcpy(buffer + 8, unwind_info, size);
        ASSERT(code_offset >= 8+size);
        memset(buffer + (8+size), 0, code_offset - (8+size));
        handle->code_len += code_offset;
    }

    /* In the worst case (Windows ABI with all registers saved and a frame
     * size of >=128 bytes), the prologue will require:
     *    1 * 4 low GPR saves
     *    2 * 4 high GPR saves
     *    7 * 1 stack adjustment
     *    8 * 2 low XMM saves
     *    9 * 8 high XMM saves
     * for a total of 107 bytes. */
    if (UNLIKELY(!binrec_ensure_code_space(handle, 107))) {
        log_error(handle, "No memory for unit prologue");
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

    if (is_windows_seh) {
        /* Make sure the prologue is the same length we said it would be. */
        const int code_offset =
            (int) *ALIGNED_CAST(uint64_t *, handle->code_buffer);
        ASSERT(code.len == code_offset + handle->code_buffer[9]);
    }

    /* If we have any local constants, insert them here with a jump over
     * them to the first instruction. */
    int num_xmm_constants = 0;
    for (int i = 0; i < lenof(ctx->const_loc); i++) {
        if (ctx->const_loc[i]) {
            num_xmm_constants++;
        }
    }
    if (num_xmm_constants) {
        const int cst_size = 16 * num_xmm_constants;
        const int padding = (16 - (code.len + (cst_size>=128 ? 5 : 2))) & 15;
        const int disp = padding + cst_size;

        handle->code_len = code.len;
        if (UNLIKELY(!binrec_ensure_code_space(handle, 5+disp))) {
            log_error(handle, "No memory for local constants");
            return false;
        }
        code.buffer = handle->code_buffer;
        code.buffer_size = handle->code_buffer_size;

        const X86Opcode jump_opcode = disp>=128 ? X86OP_JMP_Jz : X86OP_JMP_Jb;
        append_jump_raw(&code, jump_opcode, disp);
        ASSERT(code.len + padding <= code.buffer_size);
        memset(&code.buffer[code.len], 0, padding);
        code.len += padding;
        ASSERT(code.len % 16 == 0);

        for (int i = 0; i < lenof(ctx->const_loc); i++) {
            if (ctx->const_loc[i]) {
                ctx->const_loc[i] = code.len;
                ASSERT(code.len + 16 <= code.buffer_size);
                memcpy(&code.buffer[code.len], &local_constants[i], 16);
                code.len += 16;
            }
        }
    }

    handle->code_len = code.len;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * append_epilogue:  Append the function epilogue to the output code buffer.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     append_ret: True to append a RET instruction after the epilogue.
 * [Return value]
 *     True on success, false if out of memory.
 */
static bool append_epilogue(HostX86Context *ctx, bool append_ret)
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
        log_error(handle, "No memory for unit epilogue");
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

    if (append_ret) {
        append_opcode(&code, X86OP_RET);
    }

    handle->code_len = code.len;

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * do_call_setup:  Perform setup for a call-like instruction (CALL,
 * CALL_TRANSPARENT, or CHAIN).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     code: Output code buffer.
 *     insn_index: Index of instruction in ctx->unit->insns[].
 *     is_tail: True to set up for a tail call, false to set up for a
 *         non-tail call.
 *     src1_loc: X86Register holding the target address, or -1 if the
 *         address is not in a host register.  If not -1, may be modified
 *         on return.
 *     src2, src3: RTL registers holding the function arguments, or 0 if
 *         there is no argument in the corresponding position.
 */
static void do_call_setup(HostX86Context *ctx, CodeBuffer *code,
                          int insn_index,  bool is_tail,
                          int *src1_loc, int src2, int src3)
{
    const RTLUnit * const unit = ctx->unit;
    int src2_loc = (!src2 || !unit->regs[src2].live
                          || is_spilled(ctx, insn_index, src2)
                    ? -1 : ctx->regs[src2].host_reg);
    int src3_loc = (!src3 || !unit->regs[src3].live
                          || is_spilled(ctx, insn_index, src3)
                    ? -1 : ctx->regs[src3].host_reg);

    /*
     * Put arguments (if any) in the right place.  This is a bit ugly
     * due to all the different ways we might have to swap values around.
     * We take advantage of the fact that RAX and R11 are both callee-saved
     * (thus unused at this point) and not argument registers in either ABI
     * (System V or Windows), and use them as temporaries if needed.
     *
     * Several of these cases could be implemented in fewer operations
     * using swaps, but the XCHG instruction has the same latency as a
     * 3-move sequence (temp=a, a=b, b=temp) on current-generation CPUs,
     * so we prefer moves if we can get away with less than three per swap.
     *
     * Behavior table for 1 argument:
     *  src2  | src1  | Actions
     * -------+-------+----------------------------------------------------
     *  arg0  | (any) | Reload src1 to RAX (if spilled; likewise below)
     *   RAX  | arg0  | Move arg0 to R11, move RAX to arg0
     *  (any) | arg0  | Move arg0 to RAX, move/reload src2 to arg0
     *  (any) | (any) | Move/reload src2 to arg0, reload src1 to RAX
     *
     * Behavior table for 2 arguments:
     *  src2  | src3  | src1  | Actions
     * -------+-------+-------+--------------------------------------------
     *  arg0  | arg0  | arg1  | Copy arg1 to RAX, copy arg0 to arg1
     *  arg0  | arg0  | (any) | Copy arg0 to arg1, reload src1 to RAX
     *  arg0  | arg1  | (any) | Reload src1 to RAX
     *  arg0  |  RAX  | arg1  | Move arg1 to R11, move RAX to arg1
     *  arg0  | (any) | arg1  | Move arg11 to RAX, move/reload src3 to arg1
     *  arg0  | (any) | (any) | Move/reload src3 to arg1, reload src1 to RAX
     *  arg1  | arg0  | arg0  | Swap arg0 and arg1 (and call arg1)
     *  arg1  | arg0  | arg1  | Swap arg0 and arg1 (and call arg0)
     *  arg1  | arg0  | (any) | Swap arg0 and arg1, reload src1 to RAX
     *  arg1  | arg1  | arg0  | Move arg0 to RAX, copy arg1 to arg0
     *  arg1  | arg1  | (any) | Copy arg1 to arg0, reload src1 to RAX
     *  arg1  |  RAX  | arg0  | Move arg0 to R11, move arg1 to arg0, move
     *        |       |       |    RAX to arg1
     *  arg1  | (any) | arg0  | Move arg0 to RAX, move arg1 to arg0,
     *        |       |       |    move/reload src3 to arg1
     *  arg1  | (any) | arg1  | Move arg1 to arg0, move/reload src3 to arg1
     *        |       |       |    (and call arg0)
     *  arg1  | (any) | (any) | Move arg1 to arg0, move/reload src3 to arg1,
     *        |       |       |    reload src1 to RAX
     *  (any) | arg0  | arg0  | Move arg0 to arg1, move/reload src2 to arg0
     *  (any) | arg0  | arg1  | Move/reload src2 to R11, move arg1 to RAX,
     *        |       |       |    move arg0 to arg1, move R11 to arg0
     *  (any) | arg0  | (any) | Move arg0 to arg1, move/reload src2 to arg0,
     *        |       |       |    reload src1 to RAX
     *   RAX  | arg1  | arg0  | Move arg0 to R11, move RAX to arg0
     *  (any) | arg1  | arg0  | Move arg0 to RAX, move/reload src2 to arg0
     *  (any) | arg1  | (any) | Move/reload src2 to arg0, reload src1 to RAX
     *   RAX  | (any) | arg0  | Move/reload src3 to arg1, move arg0 to R11,
     *        |       |       |    move RAX to arg0
     *  (any) | (any) | arg0  | Move/reload src3 to arg1, move arg0 to RAX,
     *        |       |       |    move/reload src2 to arg0
     *  (any) |  RAX  | arg1  | Move/reload src2 to arg0, move arg1 to R11,
     *        |       |       |    move RAX to arg1
     *  (any) | (any) | arg1  | Move/reload src2 to arg0, move arg1 to RAX,
     *        |       |       |    move/reload src3 to arg1
     *  (any) | (any) | (any) | Move/reload src2 to arg0, move/reload src3
     *        |       |       |    to arg1, reload src1 to RAX
     */

    if (src2) {

        const bool src2_is64 = int_type_is_64(unit->regs[src2].type);
        const bool src3_is64 =  // Safe even if src3 == 0.
            int_type_is_64(unit->regs[src3].type);
        const int host_arg0 = host_x86_int_arg_register(ctx, 0);
        ASSERT(host_arg0 >= 0);
        const int host_arg1 = host_x86_int_arg_register(ctx, 1);
        ASSERT(host_arg1 >= 0);

        /* For simplicity, we omit the reload of src1 if that's the last
         * operation, since we fall through to a reload of src1 anyway. */

        if (!src3) {  // 1-argument call

            if (src2_loc != host_arg0) {
                if (*src1_loc == host_arg0) {
                    const X86Register src1_target =
                        (src2_loc == X86_AX) ? X86_R11 : X86_AX;
                    append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                          src1_target, host_arg0);
                    *src1_loc = src1_target;
                }
                append_move_or_load_gpr(code, ctx, unit, insn_index,
                                        host_arg0, src2);
            }

        } else {  // 2-argument call

            if (src2_loc == host_arg0) {

                if (src3_loc == host_arg0) {
                    if (*src1_loc == host_arg1) {
                        append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                              X86_AX, host_arg1);
                        *src1_loc = X86_AX;
                    }
                    append_insn_ModRM_reg(code, src2_is64, X86OP_MOV_Gv_Ev,
                                          host_arg1, host_arg0);
                } else if (src3_loc != host_arg1) {
                    if (*src1_loc == host_arg1) {
                        const X86Register src1_target =
                            (src3_loc == X86_AX) ? X86_R11 : X86_AX;
                        append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                              src1_target, host_arg1);
                        *src1_loc = src1_target;
                    }
                    append_move_or_load_gpr(code, ctx, unit, insn_index,
                                            host_arg1, src3);
                }

            } else if (src2_loc == host_arg1) {

                if (src3_loc == host_arg0) {
                    append_insn_ModRM_reg(code, src2_is64 || src3_is64,
                                          X86OP_XCHG_Ev_Gv,
                                          host_arg1, host_arg0);
                    if (*src1_loc == host_arg0) {
                        *src1_loc = host_arg1;
                    } else if (*src1_loc == host_arg1) {
                        *src1_loc = host_arg0;
                    }
                } else if (src3_loc == host_arg1) {
                    if (*src1_loc == host_arg0) {
                        append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                              X86_AX, host_arg0);
                        *src1_loc = X86_AX;
                    }
                    append_insn_ModRM_reg(code, src2_is64, X86OP_MOV_Gv_Ev,
                                          host_arg0, host_arg1);
                } else {
                    if (*src1_loc == host_arg0) {
                        const X86Register src1_target =
                            (src3_loc == X86_AX) ? X86_R11 : X86_AX;
                        append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                              src1_target, host_arg0);
                        *src1_loc = src1_target;
                    }
                    append_insn_ModRM_reg(code, src2_is64, X86OP_MOV_Gv_Ev,
                                          host_arg0, host_arg1);
                    if (*src1_loc == host_arg1) {
                        *src1_loc = host_arg0;
                    }
                    append_move_or_load_gpr(code, ctx, unit, insn_index,
                                            host_arg1, src3);
                }

            } else if (src3_loc == host_arg0) {

                if (*src1_loc == host_arg1) {
                    append_move_or_load_gpr(code, ctx, unit, insn_index,
                                            X86_R11, src2);
                    append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                          X86_AX, host_arg1);
                    *src1_loc = X86_AX;
                    append_insn_ModRM_reg(code, src3_is64, X86OP_MOV_Gv_Ev,
                                          host_arg1, host_arg0);
                    append_insn_ModRM_reg(code, src2_is64, X86OP_MOV_Gv_Ev,
                                          host_arg0, X86_R11);
                } else {
                    append_insn_ModRM_reg(code, src3_is64, X86OP_MOV_Gv_Ev,
                                          host_arg1, host_arg0);
                    if (*src1_loc == host_arg0) {
                        *src1_loc = host_arg1;
                    }
                    append_move_or_load_gpr(code, ctx, unit, insn_index,
                                            host_arg0, src2);
                }

            } else if (src3_loc == host_arg1) {

                if (*src1_loc == host_arg0) {
                    const X86Register src1_target =
                        (src2_loc == X86_AX) ? X86_R11 : X86_AX;
                    append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                          src1_target, host_arg0);
                    *src1_loc = src1_target;
                }
                append_move_or_load_gpr(code, ctx, unit, insn_index,
                                        host_arg0, src2);

            } else if (*src1_loc == host_arg0) {

                append_move_or_load_gpr(code, ctx, unit, insn_index,
                                        host_arg1, src3);
                const X86Register src1_target =
                    (src2_loc == X86_AX) ? X86_R11 : X86_AX;
                append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                      src1_target, host_arg0);
                *src1_loc = src1_target;
                append_move_or_load_gpr(code, ctx, unit, insn_index,
                                        host_arg0, src2);

            } else {

                append_move_or_load_gpr(code, ctx, unit, insn_index,
                                        host_arg0, src2);
                if (*src1_loc == host_arg1) {
                    const X86Register src1_target =
                        (src3_loc == X86_AX) ? X86_R11 : X86_AX;
                    append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev,
                                          src1_target, host_arg1);
                    *src1_loc = src1_target;
                }
                append_move_or_load_gpr(code, ctx, unit, insn_index,
                                        host_arg1, src3);

            }

        }  // if (!src3)

    }  // if (src2)
}

/*-----------------------------------------------------------------------*/

/**
 * translate_call:  Translate a CALL or CALL_TRANSPARENT instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     block_index: Index of basic block in ctx->unit->blocks[].
 *     insn_index: Index of instruction in ctx->unit->insns[].
 * [Return value]
 *     True on success, false if out of memory.
 */
static bool translate_call(HostX86Context *ctx, int block_index,
                           int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(block_index >= 0);
    ASSERT(block_index < ctx->unit->num_blocks);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);

    binrec_t * const handle = ctx->handle;
    const RTLUnit * const unit = ctx->unit;
    const RTLBlock * const block = &unit->blocks[block_index];
    RTLInsn * const insn = &unit->insns[insn_index];
    const int src1 = insn->src1;
    const int src2 = insn->src2;
    const int src3 = insn->src3;
    int src1_loc = (!unit->regs[src1].live || is_spilled(ctx, insn_index, src1)
                    ? -1 : ctx->regs[src1].host_reg);
    const bool is_tail = (insn->host_data_16 != 0);

    /* Call setup will generally require more space than is reserved by
     * default, so expand the buffer if needed. */
    const int MAX_SETUP_LEN = 3*10;  // 3x 64-bit immediate (src1/src2/src3)
    /* Tail calls: worst case epilogue (107 bytes, see append_epilogue()) +
     * JMP Ev (without REX, since src1 is loaded to RAX) */
    const int MAX_TAIL_CALL_LEN = MAX_SETUP_LEN + 107 + 2;
    /* Nontail calls: CALL Ev (without REX, since spilled or immediate src1
     * is always loaded to RAX) + return value copy (with REX) + MXCSR
     * save/load (16) + worst case save/restore for System V ABI (9x REX
     * GPR store, 8x non-REX XMM store, 7x REX XMM store, all doubled) */
    const int MAX_NONTAIL_CALL_LEN =
        MAX_SETUP_LEN + 2 + 3 + 16 + (2 * (9*8 + 8*8 + 7*9));
    const int max_len = is_tail ? MAX_TAIL_CALL_LEN : MAX_NONTAIL_CALL_LEN;
    if (UNLIKELY(handle->code_len + max_len > handle->code_buffer_size)
     && UNLIKELY(!binrec_ensure_code_space(handle, max_len))) {
        log_error(handle, "No memory for CALL instruction");
        return false;
    }

    CodeBuffer code = {.buffer = handle->code_buffer,
                       .buffer_size = handle->code_buffer_size,
                       .len = handle->code_len};
    const long initial_len = code.len;

    /* If this is not a tail call, we need to save any values live in
     * caller-saved registers to the stack (and we need to do this before
     * potentially clobbering them with function arguments).  The register
     * allocator will let us know which registers are live via the
     * host_data_32 field in the CALL instruction. */
    if (!is_tail) {
        if (insn->dest) {
            /* Make sure we don't overwrite the result after the call!
             * If the result register is in the save set, it means that
             * register spilled whatever was previously living there, so
             * remove it from the save set. */
            insn->host_data_32 &= ~(1 << ctx->regs[insn->dest].host_reg);
        }
        uint32_t save_regs = insn->host_data_32;
        while (save_regs) {
            const int reg = ctz32(save_regs);
            save_regs ^= 1 << reg;
            ASSERT(ctx->stack_callsave[reg] >= 0);
            if (reg >= X86_XMM0) {
                append_insn_ModRM_mem(&code, false, X86OP_MOVAPS_W_V, reg,
                                      X86_SP, -1, ctx->stack_callsave[reg]);
            } else {
                append_insn_ModRM_mem(&code, true, X86OP_MOV_Ev_Gv, reg,
                                      X86_SP, -1, ctx->stack_callsave[reg]);
            }
        }

        /* We also save MXCSR since its value is volatile in all x86 ABIs. */
        ASSERT(ctx->stack_mxcsr >= 0);
        append_insn_ModRM_mem(
            &code, false, X86OP_MISC_0FAE, X86OP_MISC0FAE_STMXCSR,
            X86_SP, -1, ctx->stack_mxcsr);
    }

    /* Get arguments into the right place. */
    do_call_setup(ctx, &code, insn_index, is_tail, &src1_loc, src2, src3);

    /* Reload the call target (or copy from constant), if necessary. */
    if (src1_loc < 0) {
        append_move_or_load_gpr(&code, ctx, unit, insn_index, X86_AX, src1);
        src1_loc = X86_AX;
    }

    if (is_tail) {

        /* If the call target is in a callee-saved register, it'll be
         * clobbered by the epilogue, so move it out of the way.  We use
         * RAX since it's the return register in both SysV and Windows ABIs
         * and it doesn't require a REX prefix. */
        if (ctx->callee_saved_regs & (1 << src1_loc)) {
            append_move_gpr(&code, RTLTYPE_ADDRESS, X86_AX, src1_loc);
            src1_loc = X86_AX;
        }

        /* Append the epilogue.  We've already reserved space for it, so
         * this can never fail. */
        handle->code_len = code.len;
        ASSERT(append_epilogue(ctx, false));
        ASSERT(handle->code_buffer == code.buffer);
        ASSERT(handle->code_buffer_size == code.buffer_size);
        code.len = handle->code_len;

        /* Do the actual tail call. */
        append_insn_ModRM_reg(&code, false, X86OP_MISC_FF,
                              X86OP_MISC_FF_JMP_Ev, src1_loc);

        /* The following RETURN (if present) is no longer needed. */
        if ((uint32_t)insn_index < unit->num_insns - 1) {
            /* RETURN should never start a new block. */
            ASSERT(insn_index < block->last_insn);
            unit->insns[insn_index+1].opcode = RTLOP_NOP;
            unit->insns[insn_index+1].src1 = 0;
        }

    } else {  // not a tail call

        /* Do the actual call. */
        append_insn_ModRM_reg(&code, false, X86OP_MISC_FF,
                              X86OP_MISC_FF_CALL_Ev, src1_loc);

        /* If the return value is stored to an RTL register, move it to
         * where it belongs. */
        const int dest = insn->dest;
        if (dest && ctx->regs[dest].host_reg != X86_AX) {
            append_move_gpr(&code, unit->regs[dest].type,
                            ctx->regs[dest].host_reg, X86_AX);
        }

        /* Restore all registers we saved before the call. */
        append_insn_ModRM_mem(
            &code, false, X86OP_MISC_0FAE, X86OP_MISC0FAE_LDMXCSR,
            X86_SP, -1, ctx->stack_mxcsr);
        uint32_t save_regs = insn->host_data_32;
        while (save_regs) {
            const int reg = ctz32(save_regs);
            save_regs ^= 1 << reg;
            ASSERT(ctx->stack_callsave[reg] >= 0);
            if (reg >= X86_XMM0) {
                append_insn_ModRM_mem(&code, false, X86OP_MOVAPS_V_W, reg,
                                      X86_SP, -1, ctx->stack_callsave[reg]);
            } else {
                append_insn_ModRM_mem(&code, true, X86OP_MOV_Gv_Ev, reg,
                                      X86_SP, -1, ctx->stack_callsave[reg]);
            }
        }

    }  // if (is_tail)

    ASSERT(code.len - initial_len <= max_len);
    handle->code_len = code.len;
    ctx->last_test_reg = 0;
    ctx->last_cmp_reg = 0;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * translate_chain:  Translate a CHAIN instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of instruction in ctx->unit->insns[].
 * [Return value]
 *     True on success, false if out of memory.
 */
static bool translate_chain(HostX86Context *ctx, int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);

    binrec_t * const handle = ctx->handle;
    const RTLUnit * const unit = ctx->unit;
    RTLInsn * const insn = &unit->insns[insn_index];

    /* Reserve space as for a tail call (see notes in translate_call()),
     * plus extra for the chain logic itself.  We don't attempt to
     * optimize immediates here (since there should never be a reason to),
     * and we handle the address separately, so the maximum setup length
     * from do_call_setup() is 2 REX GPR loads (8 bytes each). */
    const int MAX_CHAIN_PREFIX_LEN = 17;  // 7 bytes alignment + MOV R15,imm64
    const int CHAIN_SUFFIX_LEN = 3;  // MOV RAX,R15
    const int MAX_TAIL_CALL_LEN = 2*8 + 107 + 2;
    const int max_len =
        MAX_CHAIN_PREFIX_LEN + MAX_TAIL_CALL_LEN + CHAIN_SUFFIX_LEN;
    if (UNLIKELY(handle->code_len + max_len > handle->code_buffer_size)
     && UNLIKELY(!binrec_ensure_code_space(handle, max_len))) {
        log_error(handle, "No memory for CHAIN instruction");
        return false;
    }

    CodeBuffer code = {.buffer = handle->code_buffer,
                       .buffer_size = handle->code_buffer_size,
                       .len = handle->code_len};
    const long initial_len = code.len;

    /*
     * We start the chain with a long jump over the chain code followed by
     * five bytes of 0x00.  These will be replaced by MOV R15,addr (10
     * bytes) once the address is known.  We arrange for the jump to be
     * 64-bit aligned so that in the common case of an address with the
     * high 16 bits clear, we can write both the MOV R15 opcode and the
     * low 48 bits of the address in a single store, thus minimizing the
     * number of store operations that could potentially trigger a
     * self-modifying code condition.  (It's superficially convenient that
     * the x86 architecture requires the CPU to detect modified code, but
     * that just ends up constraining the logic for actually modifying the
     * code.)
     *
     * Those initial 10 bytes are followed by the standard call setup to
     * get arguments into their proper places.  After setup, we move the
     * address from R15 to RAX so it's not clobbered by the function
     * epilogue, then pop the stack and jump to RAX like a normal tail call.
     */

    append_nops(&code, (8 - code.len) & 7);
    ASSERT(code.len % 8 == 0);
    insn->host_data_32 = code.len;  // Save code location for CHAIN_RESOLVE.

    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, X86_R15);
    append_imm64(&code, 0);

    do_call_setup(ctx, &code, insn_index, true,
                  (int[]){-1}, insn->src1, insn->src2);

    append_move_gpr(&code, RTLTYPE_ADDRESS, X86_AX, X86_R15);

    handle->code_len = code.len;
    ASSERT(append_epilogue(ctx, false));
    ASSERT(handle->code_buffer == code.buffer);
    ASSERT(handle->code_buffer_size == code.buffer_size);
    code.len = handle->code_len;

    append_insn_ModRM_reg(&code, false, X86OP_MISC_FF,
                          X86OP_MISC_FF_JMP_Ev, X86_AX);

    /* Overwrite the MOV R15,imm64 opcode with a jump, now that we know
     * where it should land. */
    const int disp = code.len - (insn->host_data_32 + 5);
    ASSERT(disp < 256);
    code.buffer[insn->host_data_32] = X86OP_JMP_Jz;
    code.buffer[insn->host_data_32 + 1] = (uint8_t)disp;

    ASSERT(code.len - initial_len <= max_len);
    handle->code_len = code.len;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * translate_chain_resolve:  Translate a CHAIN_RESOLVE instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     code: Output code buffer.
 *     insn_index: Index of instruction in ctx->unit->insns[].
 */
static void translate_chain_resolve(HostX86Context *ctx, CodeBuffer *code,
                                    int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);

    const RTLUnit * const unit = ctx->unit;
    RTLInsn * const insn = &unit->insns[insn_index];
    /* There should never be a reason for the address to be spilled. */
    ASSERT(!is_spilled(ctx, insn_index, insn->src1));
    const X86Register host_src1 = ctx->regs[insn->src1].host_reg;

    const int target_index = insn->src_imm;
    ASSERT(target_index >= 0);
    ASSERT(target_index < insn_index);
    const RTLInsn * const target_insn = &unit->insns[target_index];
    ASSERT(target_insn->opcode == RTLOP_CHAIN);
    const long target_offset = target_insn->host_data_32;
    ASSERT(target_offset + 10 <= code->len);
    ASSERT(target_offset % 8 == 0);

    /* Don't resolve the chain if the pointer is null. */
    append_insn_ModRM_reg(code, true, X86OP_TEST_Ev_Gv, host_src1, host_src1);
    append_jump_raw(code, X86OP_JZ_Jb, 0);
    const long skip_from = code->len;

    /* If the high 16 bits of the value are nonzero, first store those to
     * the high bits of the immediate.  R15 is guaranteed to be saved by
     * the prologue due to the CHAIN instruction, so we can safely use it
     * as a temporary here. */
    append_insn_ModRM_reg(code, true, X86OP_MOV_Gv_Ev, X86_R15, host_src1);
    append_insn_ModRM_reg(code, true, X86OP_SHIFT_Ev_Ib, X86OP_SHIFT_SHR,
                          X86_R15);
    append_imm8(code, 48);
    append_jump_raw(code, X86OP_JZ_Jb, 8);
    const long low48_from = code->len;
    append_opcode(code, X86OP_OPERAND_SIZE);
    append_insn_ModRM_riprel(code, false, X86OP_MOV_Ev_Gv,
                             X86_R15, target_offset + 8);
    ASSERT(code->len == low48_from + 8);

    /* Shift the MOV R15 opcode into the bottom of the address and store
     * it over the branch.  We can safely destroy the existing value
     * because it dies here by contract (RTL mandates that a CHAIN_RESOLVE
     * instruction is immediately followed by RETURN). */
    append_insn_ModRM_reg(code, true, X86OP_SHIFT_Ev_Ib, X86OP_SHIFT_SHL,
                          host_src1);
    append_imm8(code, 16);
    append_insn_ModRM_reg(code, true, X86OP_IMM_Ev_Iz, X86OP_IMM_OR,
                          host_src1);
    append_imm32(code, X86OP_REX_WB | (X86OP_MOV_rAX_Iv + (X86_R15 & 7)) << 8);
    append_insn_ModRM_riprel(code, true, X86OP_MOV_Ev_Gv,
                             host_src1, target_offset);

    /* Jump directly to the chain code so we don't force the caller to
     * look up the translated code for the target address a second time. */
    const long disp = target_offset - code->len;
    ASSERT(disp < 0);
    if (disp-2 >= -128) {
        append_jump_raw(code, X86OP_JMP_Jb, disp-2);
    } else {
        append_jump_raw(code, X86OP_JMP_Jz, disp-5);
    }

    const long skip_to = code->len;
    ASSERT(skip_to - skip_from < 128);
    code->buffer[skip_from - 1] = (uint8_t)(skip_to - skip_from);
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fma:  Translate a fused multiply-add instruction.
 *
 * [Parameters]
 *     code: Output code buffer.
 *     ctx: Translation context.
 *     insn_index: Index of instruction in ctx->unit->insns[].
 *     fma_base_opcode: Base opcode (one of VF*132PS) when using the FMA
 *         instruction set extension.
 *     sub: True if the operation is FMSUB or FNMSUB.
 *     negate: True if the operation is FNMADD or FNMSUB.
 */
static void translate_fma(CodeBuffer *code, HostX86Context *ctx,
                          int insn_index, X86Opcode fma_base_opcode,
                          bool sub, bool negate)
{
    ASSERT(code);
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);

    const RTLUnit * const unit = ctx->unit;
    const RTLInsn * const insn = &unit->insns[insn_index];
    const int dest = insn->dest;
    const int src1 = insn->src1;
    const int src2 = insn->src2;
    const int src3 = insn->src3;
    const X86Register host_dest = ctx->regs[dest].host_reg;
    const RTLDataType type = unit->regs[dest].type;
    const bool is_vector = rtl_type_is_vector(type);
    const RTLDataType scalar_type =
        is_vector ? rtl_vector_element_type(type) : type;
    const bool is64 = (scalar_type == RTLTYPE_FLOAT64);

    if (ctx->handle->setup.host_features & BINREC_FEATURE_X86_FMA) {

        const X86Register host_src1 = ctx->regs[src1].host_reg;
        const X86Register host_src3 = ctx->regs[src3].host_reg;
        ASSERT(ctx->regs[dest].temp_allocated);
        const X86Register host_temp = ctx->regs[dest].host_temp;
        const bool spilled1 = is_spilled(ctx, insn_index, src1);
        const bool spilled3 = is_spilled(ctx, insn_index, src3);

        /* Final opcode for the operation. */
        X86Opcode opcode;
        /* RTL register copied/loaded to the destination XMM register. */
        int dest_reg;
        /* RTL register used as the effective address operand. */
        int ea_reg;
        /* X86Register which is encoded in the VEX.vvvv field. */
        X86Register vex_vvvv;
        /* True if src1 needs to be reloaded to host_temp. */
        bool reload_src1 = false;

        /* Choose the opcode variant, assign RTL registers to XMM operands,
         * and record which registers need to be reloaded. */
        if (spilled3) {
            /* src3 is spilled, so use variant 213 which adds from the
             * effective address operand. */
            opcode = fma_base_opcode + (X86OP_VFMADD213SS - X86OP_VFMADD132SS);
            dest_reg = src2;
            ea_reg = src3;
            if (spilled1) {
                reload_src1 = true;
                vex_vvvv = host_temp;
            } else {
                vex_vvvv = host_src1;
            }
        } else if (host_dest == host_src3) {
            /* dest overlaps src3, so use variant 231 which overwrites
             * the addend. */
            opcode = fma_base_opcode + (X86OP_VFMADD231SS - X86OP_VFMADD132SS);
            dest_reg = src3;
            ea_reg = src2;
            if (spilled1) {
                reload_src1 = true;
                vex_vvvv = host_temp;
            } else {
                vex_vvvv = host_src1;
            }
        } else {
            /* For all other cases, we can use variant 132. */
            opcode = fma_base_opcode;
            dest_reg = src1;
            ea_reg = src2;
            vex_vvvv = host_src3;
        }

        /* Choose between vector and scalar versions of the instruction. */
        ASSERT(!(opcode & 1));
        if (!rtl_register_is_vector(&unit->regs[dest])) {
            opcode |= 1;
        }

        /* Reload registers as needed. */
        append_move_or_load(code, ctx, unit, insn_index, host_dest, dest_reg);
        if (reload_src1) {
            append_load(code, type, host_temp,
                        X86_SP, -1, ctx->regs[src1].spill_offset);
        }

        /* Perform the actual operation. */
        append_vex_insn_ModRM_ctx(code, is64, false, opcode, host_dest,
                                  ctx, insn_index, ea_reg, vex_vvvv);

    } else {
        /* Without the FMA extension, we just do separate multiply/add
         * operations and eat the precision loss.  Yum. */

        const X86Register host_src2 = ctx->regs[src2].host_reg;
        const uint8_t prefix = sse_opcode_prefix_for_type(type);
        const X86Opcode add_opcode = sub ? X86OP_SUBPS : X86OP_ADDPS;

        if (host_dest == host_src2 && !is_spilled(ctx, insn_index, src2)) {
            if (prefix) {
                append_imm8(code, prefix);
            }
            append_insn_ModRM_ctx(code, false, X86OP_MULPS, host_dest,
                                  ctx, insn_index, src1);
        } else {
            append_move_or_load(code, ctx, unit, insn_index, host_dest, src1);
            if (prefix) {
                append_imm8(code, prefix);
            }
            append_insn_ModRM_ctx(code, false, X86OP_MULPS, host_dest,
                                  ctx, insn_index, src2);
        }

        if (negate) {
            const int lc_id = is_vector
                ? (is64 ? LC_V2_FLOAT64_SIGNBIT : LC_V2_FLOAT32_SIGNBIT)
                : (is64 ? LC_FLOAT64_SIGNBIT : LC_FLOAT32_SIGNBIT);
            const long lc_offset = ctx->const_loc[lc_id];
            ASSERT(lc_offset);
            append_insn_ModRM_riprel(code, false, X86OP_XORPS, host_dest,
                                     lc_offset);
        }

        if (prefix) {
            append_imm8(code, prefix);
        }
        append_insn_ModRM_ctx(code, false, add_opcode, host_dest,
                              ctx, insn_index, insn->src3);

    }
}

/*-----------------------------------------------------------------------*/

/**
 * translate_fzcast:  Translate an FZCAST instruction.
 *
 * [Parameters]
 *     ctx: Translation context.
 *     insn_index: Index of instruction in ctx->unit->insns[].
 * [Return value]
 *     True on success, false if out of memory.
 */
static bool translate_fzcast(HostX86Context *ctx, int insn_index)
{
    ASSERT(ctx);
    ASSERT(ctx->handle);
    ASSERT(ctx->unit);
    ASSERT(insn_index >= 0);
    ASSERT((uint32_t)insn_index < ctx->unit->num_insns);

    CodeBuffer code = {.buffer = ctx->handle->code_buffer,
                       .buffer_size = ctx->handle->code_buffer_size,
                       .len = ctx->handle->code_len};
    const long initial_len = code.len;

    const RTLUnit * const unit = ctx->unit;
    const RTLInsn * const insn = &unit->insns[insn_index];
    const int dest = insn->dest;
    const int src1 = insn->src1;
    const X86Register host_dest = ctx->regs[dest].host_reg;
    X86Register host_src1;
    if (is_spilled(ctx, insn_index, src1)) {
        ASSERT(ctx->regs[dest].temp_allocated);
        host_src1 = ctx->regs[dest].host_temp;
        append_load_gpr(&code, unit->regs[src1].type, host_src1,
                        X86_SP, ctx->regs[src1].spill_offset);
    } else {
        host_src1 = ctx->regs[src1].host_reg;
    }
    const X86Opcode opcode = (unit->regs[dest].type == RTLTYPE_FLOAT64
                              ? X86OP_CVTSI2SD : X86OP_CVTSI2SS);

    if (!int_type_is_64(unit->regs[src1].type)) {
        /* The INT32 case is easy: 32-bit values in GPRs will always have
         * the high 32 bits clear, so we can just use the value as is,
         * treating it as a 64-bit signed integer. */
        append_insn_ModRM_reg(&code, true, opcode, host_dest, host_src1);
        ctx->handle->code_len = code.len;
        return true;
    }

    /* The x86 instruction set doesn't include an unsigned conversion
     * instruction, but we can take advantage of the fact that both
     * FLOAT32 and FLOAT64 have less than 64 bits of precision and
     * shift the value right if its MSB is set. */

    ASSERT(ctx->regs[dest].temp_allocated);

    const int max_len = 73;
    if (UNLIKELY(ctx->handle->code_len + max_len
                 > ctx->handle->code_buffer_size)) {
        if (UNLIKELY(!binrec_ensure_code_space(ctx->handle, max_len))) {
            log_error(ctx->handle, "No memory for FZCAST instruction");
            return false;
        }
        code.buffer = ctx->handle->code_buffer;
        code.buffer_size = ctx->handle->code_buffer_size;
    }

    /* Check whether the MSB is set. */
    append_insn_ModRM_reg(&code, true, X86OP_TEST_Ev_Gv,
                          host_src1, host_src1);
    const long js_pos = code.len;
    append_jump_raw(&code, X86OP_JS_Jb, 0);
    const long js_end = code.len;
    ASSERT(js_end == js_pos + 2);

    /* MSB not set: simple conversion. */
    append_insn_ModRM_reg(&code, true, opcode, host_dest, host_src1);
    const long msb_set_jmp_pos = code.len;
    append_jump_raw(&code, X86OP_JMP_Jb, 0);
    const long msb_set_jmp_end = code.len;
    ASSERT(msb_set_jmp_end == msb_set_jmp_pos + 2);
    ASSERT(code.len - js_end <= 127);
    code.buffer[js_end-1] = (uint8_t)(code.len - js_end);

    /* MSB set: divide by 2 (with appropriate rounding) before conversion
     * and multiply by 2 afterward. */
    const X86Register host_temp = ctx->regs[dest].host_temp;
    if (host_src1 != host_temp) {
        append_move_gpr(&code, RTLTYPE_INT64, host_temp, host_src1);
    }
    append_insn_ModRM_mem(
        &code, false, X86OP_MISC_0FAE, X86OP_MISC0FAE_STMXCSR,
        X86_SP, -1, ctx->stack_mxcsr);
    append_insn_ModRM_reg(&code, true, X86OP_SHIFT_Ev_1, X86OP_SHIFT_SHR,
                          host_temp);
    /* RC={1,3}: round down */
    append_insn_ModRM_mem(&code, false, X86OP_BTx_Ev_Ib, X86OP_BITTEST_BT,
                          X86_SP, -1, ctx->stack_mxcsr);
    append_imm8(&code, 13);
    const long jnc0_pos = code.len;
    append_jump_raw(&code, X86OP_JNC_Jb, 0);
    const long jnc0_end = code.len;
    ASSERT(jnc0_end == jnc0_pos + 2);
    /* RC=2: round up */
    append_insn_ModRM_mem(&code, false, X86OP_BTx_Ev_Ib, X86OP_BITTEST_BT,
                          X86_SP, -1, ctx->stack_mxcsr);
    append_imm8(&code, 14);
    const long jnc1_pos = code.len;
    append_jump_raw(&code, X86OP_JNC_Jb, 0);
    const long jnc1_end = code.len;
    ASSERT(jnc1_end == jnc1_pos + 2);
    /* RC=0: round to even */
    maybe_append_empty_rex(&code, host_temp, -1, -1);
    append_insn_ModRM_reg(&code, false, X86OP_UNARY_Eb, X86OP_UNARY_TEST,
                          host_temp);
    append_imm8(&code, 1);
    const long jz_pos = code.len;
    append_jump_raw(&code, X86OP_JZ_Jb, 0);
    const long jz_end = code.len;
    ASSERT(jz_end == jz_pos + 2);
    /* Increment halved value if rounding up. */
    ASSERT(code.len - jnc1_end <= 127);
    code.buffer[jnc1_end-1] = (uint8_t)(code.len - jnc1_end);
    append_insn_ModRM_reg(&code, true, X86OP_IMM_Ev_Ib, X86OP_IMM_ADD,
                          host_temp);
    append_imm8(&code, 1);
    /* Convert rounded value and double. */
    ASSERT(code.len - jz_end <= 127);
    code.buffer[jz_end-1] = (uint8_t)(code.len - jz_end);
    ASSERT(code.len - jnc0_end <= 127);
    code.buffer[jnc0_end-1] = (uint8_t)(code.len - jnc0_end);
    append_insn_ModRM_reg(&code, true, opcode, host_dest, host_temp);
    const X86Opcode add_opcode = (unit->regs[dest].type == RTLTYPE_FLOAT64
                                  ? X86OP_ADDSD : X86OP_ADDSS);
    append_insn_ModRM_reg(&code, false, add_opcode, host_dest, host_dest);

    ASSERT(code.len - msb_set_jmp_end <= 127);
    code.buffer[msb_set_jmp_end-1] = (uint8_t)(code.len - msb_set_jmp_end);

    ctx->last_test_reg = 0;
    ctx->last_cmp_reg = 0;
    ctx->last_cmp_target = 0;
    ctx->last_cmp_imm = 0;

    ASSERT(code.len - initial_len <= max_len);
    ctx->handle->code_len = code.len;
    return true;
}

/*-----------------------------------------------------------------------*/

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

    ctx->last_test_reg = 0;
    ctx->last_cmp_reg = 0;
    ctx->last_cmp_target = 0;
    ctx->last_cmp_imm = 0;

    for (int insn_index = block->first_insn; insn_index <= block->last_insn;
         insn_index++)
    {
        const RTLInsn * const insn = &unit->insns[insn_index];
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
                log_error(handle, "No memory for instruction at %d",
                          insn_index);
                return false;
            }
            code.buffer = handle->code_buffer;
            code.buffer_size = handle->code_buffer_size;
        }

        /* This is not "const" because we rewrite it for instructions which
         * do their own buffer size management. */
        long initial_len = code.len;

        /* Evict the current occupant of the destination register if needed. */
        if (dest) {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const int spill_index = current_reg(ctx, insn_index, host_dest);
            if (spill_index) {
                const RTLRegister *spill_reg = &unit->regs[spill_index];
                const HostX86RegInfo *spill_info = &ctx->regs[spill_index];
                ASSERT(spill_info->spilled);
                ASSERT(spill_info->spill_insn == insn_index);
                append_store(&code, spill_reg->type, spill_info->host_reg,
                             X86_SP, -1, spill_info->spill_offset);
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
            /* We need to store to memory if (1) this is a terminal block,
             * (2) at least one successor block doesn't both (a) have a
             * mergeable GET_ALIAS and (b) SET_ALIAS the same alias, or
             * (3) this block or any successor block contains a non-tail
             * CALL or CALL_TRANSPARENT.  (FIXME: (3) only needs to apply
             * if the alias has bound storage, but for our purposes it
             * shouldn't make a difference.) */
            bool need_store =
                (block->exits[0] < 0 || block_info->has_nontail_call);
            for (int i = 0; !need_store && i < lenof(block->exits); i++) {
                const int successor = block->exits[i];
                if (successor >= 0) {
                    const int reg =
                        ctx->blocks[successor].alias_load[insn->alias];
                    if (!reg
                     || !ctx->regs[reg].merge_alias
                     || !ctx->blocks[successor].alias_store[insn->alias]
                     || ctx->blocks[successor].has_nontail_call) {
                        need_store = true;
                    }
                }
            }
            if (need_store) {
                if (is_spilled(ctx, insn_index, src1)) {
                    const RTLRegister *src1_reg = &unit->regs[src1];
                    const X86Register temp_reg =
                        (rtl_register_is_int(src1_reg) ? X86_R15 : X86_XMM15);
                    append_load(&code, src1_reg->type, temp_reg,
                                X86_SP, -1, ctx->regs[src1].spill_offset);
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

            /* Set the x86 condition flags based on the condition register. */
            X86CondCode condition;
            if (insn->host_data_16) {
                condition = insn->host_data_16 & 0xF;
                const int32_t cmp2 = insn->host_data_32;
                const bool cmp2_is_imm = (insn->host_data_16 & 0x10) != 0;
                if (insn->host_data_16 & 0x40) {
                    /* 0x40 implies FTESTEXC, which always has src1 of type
                     * FPSTATE and an immediate RTLFloatException src2. */
                    ASSERT(cmp2_is_imm);
                    if (!is_spilled(ctx, insn_index, insn->src3)) {
                        maybe_append_empty_rex(
                            &code, ctx->regs[insn->src3].host_reg, -1, -1);
                    }
                    append_insn_ModRM_ctx(
                        &code, false, X86OP_UNARY_Eb, X86OP_UNARY_TEST,
                        ctx, insn_index, insn->src3);
                    append_imm8(&code, rtlfexc_to_bits(cmp2));
                    ctx->last_test_reg = 0;
                    ctx->last_cmp_reg = 0;
                } else {
                    append_compare(ctx, insn_index, &code, insn->src3,
                                   cmp2_is_imm ? 0 : cmp2, cmp2,
                                   (insn->host_data_16 >> 8) & 0x1F,
                                   (condition & 0xE) == X86CC_Z,
                                   (insn->host_data_16 & 0x20) != 0, -1);
                }
            } else {
                condition = X86CC_NZ;
                append_compare(ctx, insn_index, &code, insn->src3,
                               0, 0, 0, true, false, -1);
            }

            /* Put one of the source values in the destination register, if
             * necessary.  Note that MOV does not alter flags. */
            bool dest_is_src1;
            if (host_dest == host_src1 && !is_spilled(ctx, insn_index, src1)) {
                dest_is_src1 = true;
            } else if (host_dest == host_src2
                       && !is_spilled(ctx, insn_index, src2)) {
                dest_is_src1 = false;
            } else {
                dest_is_src1 = true;
                append_move_or_load(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            }

            /* Conditionally move the other value into the register. */
            const int other = dest_is_src1 ? src2 : src1;
            if (rtl_register_is_float(&unit->regs[dest])) {
                if (!dest_is_src1) {
                    condition ^= 1;
                }
                const X86Opcode opcode = X86OP_Jcc_Jb | condition;
                append_opcode(&code, opcode);
                append_imm8(&code, 0);
                const long jump_from = code.len;
                append_move_or_load(&code, ctx, unit, insn_index,
                                    host_dest, other);
                const long jump_to = code.len;
                ASSERT(jump_to - jump_from <= 127);
                code.buffer[jump_from - 1] = jump_to - jump_from;
            } else {
                ASSERT(rtl_register_is_int(&unit->regs[dest])
                       || unit->regs[dest].type == RTLTYPE_FPSTATE);
                if (dest_is_src1) {
                    condition ^= 1;
                }
                const bool is64 = int_type_is_64(unit->regs[dest].type);
                const X86Opcode opcode = X86OP_CMOVcc | condition;
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, other);
            }
            break;
          }  // case RTLOP_SELECT

          case RTLOP_SCAST:
          case RTLOP_ZCAST: {
            const RTLDataType type_dest = unit->regs[dest].type;
            const RTLDataType type_src1 = unit->regs[src1].type;
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_src1 = ctx->regs[src1].host_reg;
            if (is_spilled(ctx, insn_index, src1)) {
                append_load_gpr(&code, type_src1, host_dest,
                                X86_SP, ctx->regs[src1].spill_offset);
                host_src1 = host_dest;
            }
            if (int_type_is_64(type_dest)) {
                if (int_type_is_64(type_src1)) {
                    if (host_dest != host_src1) {
                        append_insn_ModRM_reg(&code, true, X86OP_MOV_Ev_Gv,
                                              host_src1, host_dest);
                    }
                } else {
                    if (insn->opcode == RTLOP_SCAST) {
                        append_insn_ModRM_reg(&code, true, X86OP_MOVSXD_Gv_Ev,
                                              host_dest, host_src1);
                    } else if (host_dest != host_src1) {
                        /* We can skip the MOV if host_dest == host_src1
                         * because all 32-bit operations clear the high
                         * word of the output register. */
                        append_insn_ModRM_reg(&code, false, X86OP_MOV_Ev_Gv,
                                              host_src1, host_dest);
                    }
                }
            } else {
                /* When converting from 64 to 32 bits, we need a MOV even
                 * if the source and destination are in the same register
                 * in order to clear the high 32 bits of the register. */
                if (int_type_is_64(type_src1) || host_dest != host_src1) {
                    append_insn_ModRM_reg(&code, false, X86OP_MOV_Ev_Gv,
                                          host_src1, host_dest);
                }
            }
            break;
          }  // case RTLOP_SCAST, RTLOP_ZCAST

          case RTLOP_SEXT8:
          case RTLOP_SEXT16: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const bool is64 = int_type_is_64(unit->regs[dest].type);
            const X86Opcode opcode = (insn->opcode == RTLOP_SEXT8
                                      ? X86OP_MOVSX_Gv_Eb : X86OP_MOVSX_Gv_Ew);
            if (insn->opcode == RTLOP_SEXT8 && !is64
             && !is_spilled(ctx, insn_index, src1)) {
                maybe_append_empty_rex(&code, ctx->regs[src1].host_reg,
                                       host_dest, -1);
            }
            append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                  ctx, insn_index, src1);
            break;
          }  // case RTLOP_SEXT8, RTLOP_SEXT16

          case RTLOP_NEG:
            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                ctx->last_test_reg = dest;
                /* We could almost forward the EFLAGS result as an ordered
                 * compare against zero, but two cases would result in
                 * incorrect behavior:
                 *  - Negating a zero operand sets CF=1, which breaks the
                 *    AE (CF=0) and B (CF=1) tests.
                 *  - Negating the smallest negative integer (0x8000_0000
                 *    for INT32) sets OF=1, which breaks the signed tests
                 *    (the result would be treated as positive instead of
                 *    negative).
                 * So we have to perform an explicit compare for a
                 * subsequent SLT/SGT operation. */
                ctx->last_cmp_reg = 0;
            }
            /* Fall through to common NEG/NOT handling. */
          case RTLOP_NOT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            if (is_spilled(ctx, insn_index, src1)) {
                append_load_gpr(&code, unit->regs[src1].type, host_dest,
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
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_ADD ? X86OP_ADD_Gv_Ev :
                insn->opcode == RTLOP_SUB ? X86OP_SUB_Gv_Ev :
                insn->opcode == RTLOP_AND ? X86OP_AND_Gv_Ev :
                insn->opcode == RTLOP_OR ? X86OP_OR_Gv_Ev :
                             /* RTLOP_XOR */ X86OP_XOR_Gv_Ev);
            if (host_dest == host_src2 && !is_spilled(ctx, insn_index, src2)) {
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src1);
            } else {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src2);
            }
            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                ctx->last_test_reg = dest;
                ctx->last_cmp_reg = 0;
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
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const X86Opcode opcode = X86OP_IMUL_Gv_Ev;
            if (host_dest == host_src2 && !is_spilled(ctx, insn_index, src2)) {
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src1);
            } else {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                      ctx, insn_index, src2);
            }
            /* We don't need to test the CONDITION_CODES optimization
             * flag if we're just clearing the current state. */
            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_MUL

          case RTLOP_MULHU:
          case RTLOP_MULHS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            ASSERT(host_dest != X86_AX);

            /* If another value is already in rDX, save it away so it's not
             * clobbered.  We save all 64 bits of the register so we don't
             * have to worry about checking the type of what's there. */
            const bool dx_busy = (host_dest != X86_DX
                                  && current_reg(ctx, insn_index, X86_DX));
            bool swapped_dx = false;
            if (dx_busy) {
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
            if (host_src2 == X86_AX && !is_spilled(ctx, insn_index, src2)) {
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
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load_gpr(&code, unit->regs[src1].type, X86_AX,
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
            if (swapped_dx && !is_spilled(ctx, insn_index, multiplier)) {
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
                if (dx_busy) {
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          X86_DX, host_dest);
                } else {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, X86_DX);
                }
            }
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_AX, ctx->regs[dest].host_temp);
            }

            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_MULH[US]

          case RTLOP_DIVU:
          case RTLOP_DIVS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
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

            /* As with MULH*, save the result register's value if needed.
             * For division, we never allocate dest and src2 in the same
             * register, so we only need to XCHG if host_dest == host_src1. */
            X86Register dividend = host_src1;
            const bool ax_busy = (host_dest != X86_AX
                                  && current_reg(ctx, insn_index, X86_AX));
            if (host_dest != X86_AX && (divisor == X86_AX || ax_busy)) {
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

            if (is_spilled(ctx, insn_index, src1)) {
                append_load_gpr(&code, unit->regs[src1].type, X86_AX,
                                X86_SP, ctx->regs[src1].spill_offset);
            } else if (dividend != X86_AX) {
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      X86_AX, host_src1);
            }

            X86UnaryOpcode opcode;
            if (insn->opcode == RTLOP_DIVU) {
                append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                      X86_DX, X86_DX);
                opcode = X86OP_UNARY_DIV_rAX;
            } else {
                append_insn(&code, is64, X86OP_CWD);
                opcode = X86OP_UNARY_IDIV_rAX;
            }
            if (is_spilled(ctx, insn_index, src2)) {
                append_insn_ModRM_ctx(&code, is64, X86OP_UNARY_Ev, opcode,
                                      ctx, insn_index, src2);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev, opcode,
                                      divisor);
            }

            if (host_dest != X86_AX) {
                if (ax_busy) {
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          X86_AX, host_dest);
                } else {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, X86_AX);
                }
            }
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_DX, ctx->regs[dest].host_temp);
            }

            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_DIV[US]

          case RTLOP_MODU:
          case RTLOP_MODS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
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
            const bool dx_busy = (host_dest != X86_DX
                                  && current_reg(ctx, insn_index, X86_DX));
            if (host_dest != X86_DX) {
                if (divisor == X86_DX || dx_busy) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          host_dest, X86_DX);
                }
                if (divisor == X86_DX) {
                    divisor = host_dest;
                }
            }
            ASSERT(divisor != X86_DX);

            X86UnaryOpcode opcode;
            if (insn->opcode == RTLOP_MODU) {
                append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                      X86_DX, X86_DX);
                opcode = X86OP_UNARY_DIV_rAX;
            } else {
                append_insn(&code, is64, X86OP_CWD);
                opcode = X86OP_UNARY_IDIV_rAX;
            }
            if (is_spilled(ctx, insn_index, src2)) {
                append_insn_ModRM_ctx(&code, is64, X86OP_UNARY_Ev, opcode,
                                      ctx, insn_index, src2);
            } else {
                append_insn_ModRM_reg(&code, is64, X86OP_UNARY_Ev, opcode,
                                      divisor);
            }

            if (host_dest != X86_DX) {
                if (dx_busy) {
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          X86_DX, host_dest);
                } else {
                    append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, X86_DX);
                }
            }
            if (ctx->regs[dest].temp_allocated) {
                append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                      X86_AX, ctx->regs[dest].host_temp);
            }

            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_MOD[US]

          case RTLOP_SLL:
          case RTLOP_SRL:
          case RTLOP_SRA:
            if (handle->setup.host_features & BINREC_FEATURE_X86_BMI2) {
                const X86Register host_dest = ctx->regs[dest].host_reg;
                const bool is64 = int_type_is_64(unit->regs[src1].type);
                const X86Opcode opcode = (
                    insn->opcode == RTLOP_SLL ? X86OP_SHLX :
                    insn->opcode == RTLOP_SRL ? X86OP_SHRX :
                                 /* RTLOP_SRA */ X86OP_SARX);
                /* There's also a RORX instruction in BMI2, but it takes an
                 * immediate count instead of a register count because...
                 * uh, I dunno, I guess because Intel wanted to annoy
                 * compiler writers? */

                X86Register host_shift;
                if (is_spilled(ctx, insn_index, src2)) {
                    ASSERT(host_dest != ctx->regs[src1].host_reg
                           || is_spilled(ctx, insn_index, src1));
                    /* 32-bit load even if src2 is 64 bits wide because we
                     * only need the low 5-6 bits of the value. */
                    append_load_gpr(&code, RTLTYPE_INT32, host_dest,
                                    X86_SP, ctx->regs[src2].spill_offset);
                    host_shift = host_dest;
                } else {
                    host_shift = ctx->regs[src2].host_reg;
                }

                append_vex_insn_ModRM_ctx(
                    &code, is64, false, opcode,
                    host_dest, ctx, insn_index, src1, host_shift);
                break;
            }
            /* Otherwise fall through to non-BMI2 handling. */

          case RTLOP_ROL:
          case RTLOP_ROR: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool src2_spilled = is_spilled(ctx, insn_index, src2);
            ASSERT(host_dest != X86_CX);
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const X86ShiftOpcode opcode = (
                insn->opcode == RTLOP_SLL ? X86OP_SHIFT_SHL :
                insn->opcode == RTLOP_SRL ? X86OP_SHIFT_SHR :
                insn->opcode == RTLOP_SRA ? X86OP_SHIFT_SAR :
                insn->opcode == RTLOP_ROL ? X86OP_SHIFT_ROL :
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
                append_load_gpr(&code, RTLTYPE_INT32, X86_CX,
                                X86_SP, ctx->regs[src2].spill_offset);
            } else if (host_src2 != X86_CX) {
                if (current_reg(ctx, insn_index, X86_CX)) {
                    append_insn_ModRM_reg(&code, true, X86OP_XCHG_Ev_Gv,
                                          X86_CX, host_src2);
                    swapped_cx = true;
                } else {
                    append_insn_ModRM_reg(&code, false, X86OP_MOV_Gv_Ev,
                                          X86_CX, host_src2);
                }
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
            /* The shift instructions don't change any flags if the shift
             * count is zero, so we can't rely on the state of ZF. */
            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_{ROL,ROR} (and non-BMI2 SLL/SRL/SRA)

          case RTLOP_CLZ: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            if (handle->setup.host_features & BINREC_FEATURE_X86_LZCNT) {
                append_insn_ModRM_ctx(&code, is64, X86OP_LZCNT, host_dest,
                                      ctx, insn_index, src1);
            } else {
                ASSERT(ctx->regs[dest].temp_allocated);
                const X86Register host_temp = ctx->regs[dest].host_temp;
                append_insn_ModRM_ctx(&code, is64, X86OP_BSR, host_dest,
                                      ctx, insn_index, src1);
                append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_temp);
                append_imm32(&code, is64 ? 127 : 63);
                /* This can always be a 32-bit operation regardless of the
                 * input data type. */
                append_insn_ModRM_reg(&code, false, X86OP_CMOVZ,
                                      host_dest, host_temp);
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                      X86OP_IMM_XOR, host_dest);
                append_imm8(&code, is64 ? 63 : 31);
            }
            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                ctx->last_test_reg = dest;
                ctx->last_cmp_reg = 0;
            }
            break;
          }  // case RTLOP_CLZ

          case RTLOP_BSWAP: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            if (is_spilled(ctx, insn_index, src1)) {
                append_load_gpr(&code, unit->regs[src1].type, host_dest,
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
            const X86Opcode set_opcode = (
                insn->opcode == RTLOP_SLTU ? X86OP_SETB :
                insn->opcode == RTLOP_SLTS ? X86OP_SETL :
                insn->opcode == RTLOP_SGTU ? X86OP_SETA :
                insn->opcode == RTLOP_SGTS ? X86OP_SETG :
                             /* RTLOP_SEQ */ X86OP_SETZ);

            /* On current-generation Intel processors, XOR reg,reg followed
             * by SETcc has less latency than SETcc followed by MOVZX,
             * because the processor recognizes XOR as a zero idiom and
             * doesn't impose a partial register stall on subsequent use of
             * the target GPR.  However, XOR modifies the EFLAGS register,
             * so we can only make use of it if we're not omitting the
             * compare, and in that case we have to do the XOR first.
             * Naturally, this implies we also can't use XOR if the
             * destination register overlaps either of the source registers. */
            const bool should_clear_dest =
                (!is_spilled(ctx, insn_index, src1)
                 && host_dest != ctx->regs[src1].host_reg
                 && (is_spilled(ctx, insn_index, src2)
                     || host_dest != ctx->regs[src2].host_reg));
            /* We don't bother checking for SEQ since icmp_eq is only used
             * for register-immediate compares. */
            const bool added_compare = append_compare(
                ctx, insn_index, &code, src1, src2, 0, host_dest, false, false,
                should_clear_dest ? (int)host_dest : -1);
            const bool cleared_dest = should_clear_dest && added_compare;

            /* Registers SP-DI require a REX prefix (even if empty) to
             * access the low byte as a byte register. */
            maybe_append_empty_rex(&code, host_dest, -1, -1);
            append_insn_ModRM_reg(&code, false, set_opcode, 0, host_dest);
            if (!cleared_dest) {
                maybe_append_empty_rex(&code, host_dest, -1, -1);
                append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Eb,
                                      host_dest, host_dest);
            }
            break;
          }  // case RTLOP_{SEQ,SLTU,SLTS,SGTU,SGTS}

          case RTLOP_BFEXT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const int operand_size = is64 ? 64 : 32;

            /* Despite documentation suggesting otherwise, it turns out
             * BEXTR has slightly less latency than a MOV/SHR/AND sequence
             * even including the extra instruction to load the control
             * byte,  so we use it if extracting from the middle of a
             * register. */
            if ((handle->setup.host_features & BINREC_FEATURE_X86_BMI1)
             && insn->bitfield.start != 0
             && insn->bitfield.start + insn->bitfield.count < operand_size) {
                /* For this case, we use dest as a temporary to hold the
                 * control byte, so it needs to be separate from src1. */
                ASSERT(host_dest != host_src1
                       || is_spilled(ctx, insn_index, src1));
                append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_dest);
                append_imm32(&code,
                             insn->bitfield.start | insn->bitfield.count << 8);
                append_vex_insn_ModRM_ctx(
                    &code, is64, false, X86OP_BEXTR,
                    host_dest, ctx, insn_index, src1, host_dest);
                if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                    ctx->last_test_reg = dest;
                    ctx->last_cmp_reg = 0;
                }
                break;
            }

            X86Register host_shifted;
            if (insn->bitfield.start != 0) {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_dest);
                append_imm8(&code, insn->bitfield.start);
                host_shifted = host_dest;
            } else if (is_spilled(ctx, insn_index, src1)) {
                append_load_gpr(&code, unit->regs[src1].type, host_dest,
                                X86_SP, ctx->regs[src1].spill_offset);
                host_shifted = host_dest;
            } else {
                host_shifted = host_src1;
            }

            if (insn->bitfield.start + insn->bitfield.count < operand_size) {
                if (insn->bitfield.count < 8) {
                    if (host_shifted != host_dest) {
                        append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                              host_dest, host_shifted);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                          X86OP_IMM_AND, host_dest);
                    append_imm8(&code, (1 << insn->bitfield.count) - 1);
                } else if (insn->bitfield.count == 8) {
                    maybe_append_empty_rex(&code, host_shifted, host_dest, -1);
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
                    append_imm32(&code, (1 << insn->bitfield.count) - 1);
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
                    append_imm64(&code,
                                 (UINT64_C(1) << insn->bitfield.count) - 1);
                    append_insn_ModRM_reg(&code, is64, X86OP_AND_Gv_Ev,
                                          host_dest, host_andsrc);
                }
            } else if (host_dest != host_shifted) {
                /* This implies that the instruction is "extracting" the
                 * entire register contents. */
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_shifted);
            }

            /* Whether flags are set depends on the location of the
             * bitfield in the source value.  There probably aren't many
             * cases in which we'll want to save flags from a BFEXT anyway,
             * so we don't bother with the details and just clear the
             * cached state unconditionally. */
            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_BFEXT

          case RTLOP_BFINS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const X86Register host_src2 = ctx->regs[src2].host_reg;
            const bool src2_spilled = is_spilled(ctx, insn_index, src2);
            ASSERT(host_dest != host_src2 || src2_spilled);
            const bool is64 = int_type_is_64(unit->regs[src1].type);
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
                 && !is_spilled(ctx, insn_index, src1)) {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    const X86Register host_temp = ctx->regs[dest].host_temp;
                    ASSERT(host_temp != host_src2);
                    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_temp);
                    append_imm64(&code, src1_mask);
                    append_insn_ModRM_reg(&code, true, X86OP_AND_Gv_Ev,
                                          host_dest, host_temp);
                } else {
                    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_dest);
                    append_imm64(&code, src1_mask);
                    append_insn_ModRM_ctx(&code, true, X86OP_AND_Gv_Ev,
                                          host_dest, ctx, insn_index, src1);
                }
            } else {
                const uint32_t src2_mask = (1 << insn->bitfield.count) - 1;
                const uint32_t src1_mask = ~(src2_mask << insn->bitfield.start);
                if (src1_mask == 0x000000FF) {
                    if (!is_spilled(ctx, insn_index, src1)) {
                        maybe_append_empty_rex(&code, host_src1,
                                               host_dest, -1);
                    }
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
                    append_imm8(&code, (1 << insn->bitfield.count) - 1);
                } else if (insn->bitfield.count == 8) {
                    if (!src2_spilled) {
                        maybe_append_empty_rex(&code, host_src2,
                                               host_newbits, -1);
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
                    append_imm32(&code, (1 << insn->bitfield.count) - 1);
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

            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_BFINS

          case RTLOP_ANDI:
            /* AND with 255 or 65535 can be translated to a zero-extend. */
            if (insn->src_imm == 0xFF) {
                const X86Register host_dest = ctx->regs[dest].host_reg;
                const X86Register host_src1 = ctx->regs[src1].host_reg;
                if (!is_spilled(ctx, insn_index, src1)) {
                    maybe_append_empty_rex(&code, host_src1, host_dest, -1);
                }
                append_insn_ModRM_ctx(&code, false, X86OP_MOVZX_Gv_Eb,
                                      host_dest, ctx, insn_index, src1);
                break;
            } else if (insn->src_imm == 0xFFFF) {
                const X86Register host_dest = ctx->regs[dest].host_reg;
                append_insn_ModRM_ctx(&code, false, X86OP_MOVZX_Gv_Ew,
                                      host_dest, ctx, insn_index, src1);
                break;
            }
            /* Fall through to common ALU-immediate handling. */
          case RTLOP_ADDI:
          case RTLOP_ORI:
          case RTLOP_XORI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            /* The immediate value is actually signed, but we treat it as
             * unsigned here to simplify range testing. */
            const uint32_t imm = (uint32_t)insn->src_imm;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const X86ImmOpcode opcode = (
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
            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                ctx->last_test_reg = dest;
                ctx->last_cmp_reg = 0;
            }
            break;
          }  // case RTLOP_{ADDI,ANDI,ORI,XORI}

          case RTLOP_MULI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const uint32_t imm = (uint32_t)insn->src_imm;  // As for ADDI etc.
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            if (imm + 128 < 256) {
                append_insn_ModRM_ctx(&code, is64, X86OP_IMUL_Gv_Ev_Ib,
                                      host_dest, ctx, insn_index, src1);
                append_imm8(&code, (uint8_t)imm);
            } else {
                append_insn_ModRM_ctx(&code, is64, X86OP_IMUL_Gv_Ev_Iz,
                                      host_dest, ctx, insn_index, src1);
                append_imm32(&code, imm);
            }
            ctx->last_test_reg = 0;
            ctx->last_cmp_reg = 0;
            break;
          }  // case RTLOP_MULI

          case RTLOP_SLLI:
          case RTLOP_SRLI:
          case RTLOP_SRAI:
          case RTLOP_RORI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const uint8_t shift_count = (uint8_t)insn->src_imm;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const X86ShiftOpcode opcode = (
                insn->opcode == RTLOP_SLLI ? X86OP_SHIFT_SHL :
                insn->opcode == RTLOP_SRLI ? X86OP_SHIFT_SHR :
                insn->opcode == RTLOP_SRAI ? X86OP_SHIFT_SAR :
                             /* RTLOP_RORI */ X86OP_SHIFT_ROR);
            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            append_insn_ModRM_reg(&code, is64, X86OP_SHIFT_Ev_Ib,
                                  opcode, host_dest);
            append_imm8(&code, shift_count);
            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                if ((shift_count & 0xFF) != 0) {
                    ctx->last_test_reg = dest;
                } else {
                    ctx->last_test_reg = 0;
                }
                ctx->last_cmp_reg = 0;
            }
            break;
          }  // case RTLOP_{SLLI,SRLI,SRAI,RORI}

          case RTLOP_SEQI:
          case RTLOP_SLTUI:
          case RTLOP_SLTSI:
          case RTLOP_SGTUI:
          case RTLOP_SGTSI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Opcode set_opcode = (
                insn->opcode == RTLOP_SLTUI ? X86OP_SETB :
                insn->opcode == RTLOP_SLTSI ? X86OP_SETL :
                insn->opcode == RTLOP_SGTUI ? (insn->src_imm == 0
                                               ? X86OP_SETNZ : X86OP_SETA) :
                insn->opcode == RTLOP_SGTSI ? X86OP_SETG :
                             /* RTLOP_SEQI */ X86OP_SETZ);

            /* See comments in the SEQ/SLTU/SLTS/SGTU/SGTS case for why we
             * conditionally use XOR to clear dest. */
            const bool should_clear_dest =
                (is_spilled(ctx, insn_index, src1)
                 || host_dest != ctx->regs[src1].host_reg);
            const bool added_compare = append_compare(
                ctx, insn_index, &code, src1, 0, insn->src_imm, 0,
                set_opcode==X86OP_SETZ || set_opcode==X86OP_SETNZ, false,
                should_clear_dest ? (int)host_dest : -1);
            const bool cleared_dest = should_clear_dest && added_compare;

            maybe_append_empty_rex(&code, host_dest, -1, -1);
            append_insn_ModRM_reg(&code, false, set_opcode, 0, host_dest);
            if (!cleared_dest) {
                maybe_append_empty_rex(&code, host_dest, -1, -1);
                append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Eb,
                                      host_dest, host_dest);
            }
            break;
          }  // case RTLOP_{SEQI,SLTUI,SLTSI,SGTUI,SGTSI}

          case RTLOP_BITCAST: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            switch (unit->regs[src1].type) {
              case RTLTYPE_INT32:
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load(&code, RTLTYPE_FLOAT32, host_dest,
                                X86_SP, -1, ctx->regs[src1].spill_offset);
                } else {
                    append_insn_ModRM_ctx(&code, false, X86OP_MOVD_V_E,
                                          host_dest, ctx, insn_index, src1);
                }
                break;
              case RTLTYPE_INT64:
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load(&code, RTLTYPE_FLOAT64, host_dest,
                                X86_SP, -1, ctx->regs[src1].spill_offset);
                } else {
                    append_insn_ModRM_ctx(&code, true, X86OP_MOVD_V_E,
                                          host_dest, ctx, insn_index, src1);
                }
                break;
              case RTLTYPE_FLOAT32:
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load_gpr(&code, RTLTYPE_INT32, host_dest,
                                    X86_SP, ctx->regs[src1].spill_offset);
                } else {
                    append_insn_ModRM_reg(&code, false, X86OP_MOVD_E_V,
                                          ctx->regs[src1].host_reg, host_dest);
                }
                break;
              case RTLTYPE_FLOAT64:
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load_gpr(&code, RTLTYPE_INT64, host_dest,
                                    X86_SP, ctx->regs[src1].spill_offset);
                } else {
                    append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                          ctx->regs[src1].host_reg, host_dest);
                }
                break;
              default:
                log_error(handle, "Invalid data type %s in BITCAST at %d",
                          rtl_type_name(unit->regs[src1].type), insn_index);
            }
            break;
          }  // case RTLOP_BITCAST

          case RTLOP_FCVT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            if (unit->regs[dest].type == RTLTYPE_FLOAT64) {
                ASSERT(unit->regs[src1].type == RTLTYPE_FLOAT32);
                append_insn_ModRM_ctx(&code, false, X86OP_CVTSS2SD, host_dest,
                                      ctx, insn_index, src1);
            } else {
                ASSERT(unit->regs[dest].type == RTLTYPE_FLOAT32);
                ASSERT(unit->regs[src1].type == RTLTYPE_FLOAT64);
                append_insn_ModRM_ctx(&code, false, X86OP_CVTSD2SS, host_dest,
                                      ctx, insn_index, src1);
            }
            break;
          }  // case RTLOP_FCVT

          case RTLOP_FZCAST:
            handle->code_len = code.len;
            if (!translate_fzcast(ctx, insn_index)) {
                return false;
            }
            code.buffer = handle->code_buffer;
            code.buffer_size = handle->code_buffer_size;
            code.len = handle->code_len;
            initial_len = code.len;  // Suppress output length check.
            break;

          case RTLOP_FSCAST: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const bool is64 = int_type_is_64(unit->regs[src1].type);
            const X86Opcode opcode = (unit->regs[dest].type == RTLTYPE_FLOAT64
                                      ? X86OP_CVTSI2SD : X86OP_CVTSI2SS);
            append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                  ctx, insn_index, src1);
            break;
          }  // case RTLOP_FSCAST

          case RTLOP_FROUNDI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const bool is64 = int_type_is_64(unit->regs[dest].type);
            const X86Opcode opcode = (unit->regs[src1].type == RTLTYPE_FLOAT64
                                      ? X86OP_CVTSD2SI : X86OP_CVTSS2SI);
            append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                  ctx, insn_index, src1);
            break;
          }  // case RTLOP_FROUNDI

          case RTLOP_FTRUNCI: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const bool is64 = int_type_is_64(unit->regs[dest].type);
            const X86Opcode opcode = (unit->regs[src1].type == RTLTYPE_FLOAT64
                                      ? X86OP_CVTTSD2SI : X86OP_CVTTSS2SI);
            append_insn_ModRM_ctx(&code, is64, opcode, host_dest,
                                  ctx, insn_index, src1);
            break;
          }  // case RTLOP_FTRUNCI

          case RTLOP_FNEG:
          case RTLOP_FABS:
          case RTLOP_FNABS: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const bool isvec = rtl_register_is_vector(&unit->regs[dest]);
            const RTLDataType base_type = isvec
                ? rtl_vector_element_type(unit->regs[dest].type)
                : unit->regs[dest].type;
            const bool is64 = (base_type == RTLTYPE_FLOAT64);
            const X86Opcode opcode = (
                insn->opcode == RTLOP_FNEG ? X86OP_XORPS :
                insn->opcode == RTLOP_FABS ? X86OP_ANDPS :
                            /* RTLOP_FNABS */ X86OP_ORPS);
            const int lc_id =
                (insn->opcode == RTLOP_FABS
                 ? (isvec
                    ? (is64 ? LC_V2_FLOAT64_INV_SIGNBIT : LC_V2_FLOAT32_INV_SIGNBIT)
                    : (is64 ? LC_FLOAT64_INV_SIGNBIT : LC_FLOAT32_INV_SIGNBIT))
                 : (isvec
                    ? (is64 ? LC_V2_FLOAT64_SIGNBIT : LC_V2_FLOAT32_SIGNBIT)
                    : (is64 ? LC_FLOAT64_SIGNBIT : LC_FLOAT32_SIGNBIT)));
            const long lc_offset = ctx->const_loc[lc_id];
            ASSERT(lc_offset);
            append_move_or_load(&code, ctx, unit, insn_index, host_dest, src1);
            append_insn_ModRM_riprel(&code, false, opcode, host_dest,
                                     lc_offset);
            break;
          }  // case RTLOP_FNEG, RTLOP_FABS, RTLOP_FNABS

          case RTLOP_FADD:
          case RTLOP_FSUB:
          case RTLOP_FMUL:
          case RTLOP_FDIV: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_src2 = ctx->regs[src2].host_reg;
            bool src2_loaded = !is_spilled(ctx, insn_index, src2);
            const X86Opcode base_opcode = (
                insn->opcode==RTLOP_FADD ? X86OP_ADDPS :
                insn->opcode==RTLOP_FSUB ? X86OP_SUBPS :
                insn->opcode==RTLOP_FMUL ? X86OP_MULPS :
                          /* RTLOP_FDIV */ X86OP_DIVPS);
            const uint8_t prefix =
                sse_opcode_prefix_for_type(unit->regs[dest].type);

            if (insn->opcode == RTLOP_FDIV
             && unit->regs[dest].type == RTLTYPE_V2_FLOAT32) {
                /* If we do a DIVPS directly on the register values, the
                 * two high elements of the XMM vector will trigger
                 * invalid-operation exceptions since they're always zero
                 * for V2_FLOAT32.  To avoid this, we copy src2 into a
                 * temporary register and insert 1.0f in the two high
                 * elements, then divide by that temporary register
                 * instead of the original src2.  This also conveniently
                 * leaves zeroes in the high words of the output. */
                ASSERT(ctx->regs[dest].temp_allocated);
                const X86Register host_temp = ctx->regs[dest].host_temp;
                append_move_or_load(&code, ctx, unit, insn_index,
                                    host_temp, src2);
                const long lc_offset = ctx->const_loc[LC_V2_FLOAT32_HIGH_ONES];
                ASSERT(lc_offset);
                append_insn_ModRM_riprel(&code, false, X86OP_ORPS, host_temp,
                                         lc_offset);
                host_src2 = host_temp;
                src2_loaded = true;
            }

            if (host_dest == host_src2 && src2_loaded) {
                if (prefix) {
                    append_imm8(&code, prefix);
                }
                append_insn_ModRM_ctx(&code, false, base_opcode, host_dest,
                                      ctx, insn_index, src1);
            } else {
                append_move_or_load(&code, ctx, unit, insn_index,
                                    host_dest, src1);
                if (prefix) {
                    append_imm8(&code, prefix);
                }
                /* We can't use append_insn_ModRM_ctx() because src2 might
                 * be in a different register due to the FDIV hack. */
                if (src2_loaded) {
                    append_insn_ModRM_reg(&code, false, base_opcode,
                                          host_dest, host_src2);
                } else {
                    append_insn_ModRM_mem(
                        &code, false, base_opcode, host_dest,
                        X86_SP, -1, ctx->regs[src2].spill_offset);
                }
            }
            break;
          }  // case RTLOP_FADD, RTLOP_FSUB, RTLOP_FMUL, RTLOP_FDIV

          case RTLOP_FSQRT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const uint8_t prefix =
                sse_opcode_prefix_for_type(unit->regs[dest].type);
            if (prefix) {
                append_imm8(&code, prefix);
            }
            append_insn_ModRM_ctx(&code, false, X86OP_SQRTPS, host_dest,
                                  ctx, insn_index, src1);
            break;
          }  // case RTLOP_FSQRT

          case RTLOP_FCMP: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            RTLFloatCompare cmpsel = insn->fcmp & 7;
            /* LT/LE are converted to GT/GE during the first pass. */
            ASSERT(cmpsel != RTLFCMP_LT && cmpsel != RTLFCMP_LE);
            const bool do_compare =
                !(ctx->last_cmp_reg == src1 && ctx->last_cmp_target == src2);
            const bool invert = (insn->fcmp & RTLFCMP_INVERT) != 0;

            /* Reload src1 (if necessary) before any XOR to increase code
             * parallelism. */
            X86Register host_src1;
            if (do_compare) {
                if (!is_spilled(ctx, insn_index, src1)) {
                    host_src1 = ctx->regs[src1].host_reg;
                } else {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    host_src1 = ctx->regs[dest].host_temp;
                    append_load(&code, unit->regs[src1].type, host_src1,
                                X86_SP, -1, ctx->regs[src1].spill_offset);
                }
            }

            /* For EQ, we need to set dest to a default value since the
             * test requires two steps (P=0 and Z=1).  XOR reg,reg is
             * faster than MOV reg,imm for this, but if using XOR we have
             * to do it before the test so as not to clobber the test
             * result.  As long as we're at it, we also use XOR here for
             * non-EQ tests if the compare won't be omitted, since
             * XOR+SETcc is faster than SETcc+MOVZX. */
            bool dest_initted = false;
            if (do_compare && !(cmpsel == RTLFCMP_EQ && invert)) {
                append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                      host_dest, host_dest);
                dest_initted = true;
            }

            if (do_compare) {
                const bool is64 = (unit->regs[src1].type == RTLTYPE_FLOAT64);
                const bool ordered = (insn->fcmp & RTLFCMP_ORDERED) != 0;
                const X86Opcode cmp_opcode = is64
                    ? (ordered ? X86OP_COMISD : X86OP_UCOMISD)
                    : (ordered ? X86OP_COMISS : X86OP_UCOMISS);
                append_insn_ModRM_ctx(&code, false, cmp_opcode, host_src1,
                                      ctx, insn_index, src2);
                if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                    ctx->last_test_reg = 0;
                    ctx->last_cmp_reg = src1;
                    ctx->last_cmp_target = src2;
                }
            }

            if (cmpsel == RTLFCMP_EQ) {
                if (!dest_initted) {
                    append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_dest);
                    append_imm32(&code, invert ? 1 : 0);
                }
                const int jump_disp = (host_dest >= X86_SP ? 4 : 3);
                append_jump_raw(&code, X86OP_JP_Jb, jump_disp);
                const long jump_from = code.len;
                const X86Opcode set_opcode = invert ? X86OP_SETNZ : X86OP_SETZ;
                maybe_append_empty_rex(&code, host_dest, -1, -1);
                append_insn_ModRM_reg(&code, false, set_opcode, 0, host_dest);
                const long jump_to = code.len;
                ASSERT(jump_to - jump_from == jump_disp);
            } else {
                const X86Opcode set_opcode =
                    (cmpsel==RTLFCMP_GT ? (invert ? X86OP_SETBE : X86OP_SETA) :
                     cmpsel==RTLFCMP_GE ? (invert ? X86OP_SETB : X86OP_SETAE) :
                           /*RTLFCMP_UN*/ (invert ? X86OP_SETNP : X86OP_SETP));
                maybe_append_empty_rex(&code, host_dest, -1, -1);
                append_insn_ModRM_reg(&code, false, set_opcode, 0, host_dest);
                if (!dest_initted) {
                    maybe_append_empty_rex(&code, host_dest, -1, -1);
                    append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Eb,
                                          host_dest, host_dest);
                }
            }
            break;
          }  // case RTLOP_FCMP

          case RTLOP_FMADD:
            translate_fma(&code, ctx, insn_index,
                          X86OP_VFMADD132PS, false, false);
            break;

          case RTLOP_FMSUB:
            translate_fma(&code, ctx, insn_index,
                          X86OP_VFMSUB132PS, true, false);
            break;

          case RTLOP_FNMADD:
            translate_fma(&code, ctx, insn_index,
                          X86OP_VFNMADD132PS, false, true);
            break;

          case RTLOP_FNMSUB:
            translate_fma(&code, ctx, insn_index,
                          X86OP_VFNMSUB132PS, true, true);
            break;

          case RTLOP_FGETSTATE:
            append_insn_ModRM_mem(
                &code, false, X86OP_MISC_0FAE, X86OP_MISC0FAE_STMXCSR,
                X86_SP, -1, ctx->stack_mxcsr);
            append_insn_ModRM_mem(
                &code, false, X86OP_MOV_Gv_Ev, ctx->regs[dest].host_reg,
                X86_SP, -1, ctx->stack_mxcsr);
            break;

          case RTLOP_FSETSTATE:
            if (is_spilled(ctx, insn_index, src1)) {
                append_insn_ModRM_mem(
                    &code, false, X86OP_MISC_0FAE, X86OP_MISC0FAE_LDMXCSR,
                    X86_SP, -1, ctx->regs[src1].spill_offset);
            } else {
                append_store(&code, RTLTYPE_INT32, ctx->regs[src1].host_reg,
                             X86_SP, -1, ctx->stack_mxcsr);
                append_insn_ModRM_mem(
                    &code, false, X86OP_MISC_0FAE, X86OP_MISC0FAE_LDMXCSR,
                    X86_SP, -1, ctx->stack_mxcsr);
            }
            break;

          case RTLOP_FTESTEXC: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            const X86Register host_src1 = ctx->regs[src1].host_reg;
            const uint8_t bits = rtlfexc_to_bits(insn->src_imm);
            if (UNLIKELY(!bits)) {
                log_error(handle, "Invalid FP exception %d in FTESTEXC at %d",
                          (int)insn->src_imm, insn_index);
                break;
            }

            if (bits == 0x01) {
                /* In this case, we can just AND the value with 1. */
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                      X86OP_IMM_AND, host_dest);
                append_imm8(&code, 1);
                if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                    ctx->last_test_reg = dest;
                    ctx->last_cmp_reg = 0;
                }

            } else {
                bool cleared_dest = false;
                if (is_spilled(ctx, insn_index, src1)
                 || host_dest != ctx->regs[src1].host_reg) {
                    append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                          host_dest, host_dest);
                    cleared_dest = true;
                }

                if (is_spilled(ctx, insn_index, src1)) {
                    append_insn_ModRM_mem(&code, false, X86OP_UNARY_Eb,
                                          X86OP_UNARY_TEST, X86_SP, -1,
                                          ctx->regs[src1].spill_offset);
                } else {
                    maybe_append_empty_rex(&code, host_src1, -1, -1);
                    append_insn_ModRM_reg(&code, false, X86OP_UNARY_Eb,
                                          X86OP_UNARY_TEST, host_src1);
                }
                append_imm8(&code, bits);
                ctx->last_test_reg = 0;
                ctx->last_cmp_reg = 0;
                ctx->last_cmp_target = 0;
                ctx->last_cmp_imm = 0;

                maybe_append_empty_rex(&code, host_dest, -1, -1);
                append_insn_ModRM_reg(&code, false, X86OP_SETNZ, 0, host_dest);
                if (!cleared_dest) {
                    maybe_append_empty_rex(&code, host_dest, -1, -1);
                    append_insn_ModRM_reg(&code, false, X86OP_MOVZX_Gv_Eb,
                                          host_dest, host_dest);
                }
            }
            break;
          }  // case RTLOP_FTESTEXC

          case RTLOP_FCLEAREXC: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Ib,
                                  X86OP_IMM_AND, host_dest);
            append_imm8(&code, -64);
            break;
          }  // case RTLOP_FCLEAREXC

          case RTLOP_FSETROUND: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    host_dest, src1);
            append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                  X86OP_IMM_AND, host_dest);
            append_imm32(&code, 0x9FFF);
            if (insn->src_imm != RTLFROUND_NEAREST) {
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                      X86OP_IMM_OR, host_dest);
                append_imm32(
                    &code,
                    ((const uint8_t[]){0,3,1,2})[insn->src_imm & 3] << 13);
            }
            break;
          }  // case RTLOP_FSETROUND

          case RTLOP_FCOPYROUND: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            ASSERT(ctx->regs[dest].temp_allocated);
            const X86Register host_temp = ctx->regs[dest].host_temp;
            if (!is_spilled(ctx, insn_index, src2)
             && host_dest == ctx->regs[src2].host_reg) {
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                      X86OP_IMM_AND, host_dest);
                append_imm32(&code, 0x6000);
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_temp, src1);
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                      X86OP_IMM_AND, host_temp);
                append_imm32(&code, 0x9FFF);
            } else {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_dest, src1);
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                      X86OP_IMM_AND, host_dest);
                append_imm32(&code, 0x9FFF);
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        host_temp, src2);
                append_insn_ModRM_reg(&code, false, X86OP_IMM_Ev_Iz,
                                      X86OP_IMM_AND, host_temp);
                append_imm32(&code, 0x6000);
            }
            append_insn_ModRM_reg(&code, false, X86OP_OR_Gv_Ev,
                                  host_dest, host_temp);
            break;
          }  // case RTLOP_FCOPYROUND

          case RTLOP_VBUILD2: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            append_move_or_load(&code, ctx, unit, insn_index, host_dest, src1);
            if (unit->regs[dest].type == RTLTYPE_V2_FLOAT32) {
                X86Register host_src2;
                if (is_spilled(ctx, insn_index, src2)) {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    host_src2 = ctx->regs[dest].host_temp;
                    append_load(&code, RTLTYPE_FLOAT32, host_src2,
                                X86_SP, -1, ctx->regs[src2].spill_offset);
                } else {
                    host_src2 = ctx->regs[src2].host_reg;
                }
                append_insn_ModRM_reg(&code, false, X86OP_UNPCKLPS,
                                      host_dest, host_src2);
                /* Ensure that the high half of the register is clear,
                 * so that later floating-point operations don't raise
                 * unnecessary exceptions. */
                append_insn_ModRM_reg(&code, false, X86OP_MOVQ_V_W,
                                      host_dest, host_dest);
            } else {
                ASSERT(unit->regs[dest].type == RTLTYPE_V2_FLOAT64);
                append_insn_ModRM_ctx(&code, false, X86OP_MOVHPS_V_M,
                                      host_dest, ctx, insn_index, src2);
            }
            break;
          }  // case RTLOP_VBUILD2

          case RTLOP_VBROADCAST: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            if (unit->regs[src1].source == RTLREG_CONSTANT
             && unit->regs[src1].value.i64 == 0) {
                append_insn_ModRM_reg(&code, false, X86OP_XORPS,
                                      host_dest, host_dest);
            } else if (unit->regs[dest].type == RTLTYPE_V2_FLOAT32) {
                append_move_or_load(&code, ctx, unit, insn_index,
                                    host_dest, src1);
                append_insn_ModRM_reg(&code, false, X86OP_UNPCKLPS,
                                      host_dest, host_dest);
                /* If we loaded src1 from memory, the rest of the register
                 * will have been cleared, so we don't need to manually
                 * clear the high half here. */
                if (!is_spilled(ctx, insn_index, src1)) {
                    append_insn_ModRM_reg(&code, false, X86OP_MOVQ_V_W,
                                          host_dest, host_dest);
                }
            } else {
                ASSERT(unit->regs[dest].type == RTLTYPE_V2_FLOAT64);
                append_insn_ModRM_ctx(&code, false, X86OP_MOVDDUP,
                                      host_dest, ctx, insn_index, src1);
            }
            break;
          }  // case RTLOP_VBROADCAST

          case RTLOP_VEXTRACT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            if (insn->elem == 0) {
                /* Can't use append_move_or_load() since we change the type. */
                const RTLDataType element_type =
                    rtl_vector_element_type(unit->regs[src1].type);
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load(&code, element_type, host_dest,
                                X86_SP, -1, ctx->regs[src1].spill_offset);
                } else if (ctx->regs[src1].host_reg != host_dest) {
                    append_move(&code, element_type, host_dest,
                                ctx->regs[src1].host_reg);
                }
            } else if (unit->regs[src1].type == RTLTYPE_V2_FLOAT32) {
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load(&code, RTLTYPE_FLOAT32, host_dest,
                                X86_SP, -1, ctx->regs[src1].spill_offset + 4);
                } else {
                    if (ctx->regs[src1].host_reg != host_dest) {
                        append_move(&code, RTLTYPE_V2_FLOAT32, host_dest,
                                    ctx->regs[src1].host_reg);
                    }
                    append_insn_ModRM_reg(&code, false, X86OP_PSHIFTQ_U_I,
                                          X86OP_PSHIFT_SRLDQ, host_dest);
                    append_imm8(&code, 4);
                }
            } else {
                ASSERT(unit->regs[src1].type == RTLTYPE_V2_FLOAT64);
                /* We can't just call append_insn_ModRM_ctx() unconditionally
                 * with MOVLPS (a.k.a. MOVHLPS) because that reads from the
                 * second doubleword of an XMM register but the _first_
                 * doubleword at a memory address. */
                if (is_spilled(ctx, insn_index, src1)) {
                    append_load(&code, RTLTYPE_FLOAT64, host_dest,
                                X86_SP, -1, ctx->regs[src1].spill_offset + 8);
                } else {
                    append_insn_ModRM_reg(&code, false, X86OP_MOVHLPS,
                                          host_dest, ctx->regs[src1].host_reg);
                }
            }
            break;
          }  // case RTLOP_VEXTRACT

          case RTLOP_VINSERT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            append_move_or_load(&code, ctx, unit, insn_index, host_dest, src1);
            if (unit->regs[dest].type == RTLTYPE_V2_FLOAT32) {
                X86Register host_src2;
                if (is_spilled(ctx, insn_index, src2)) {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    host_src2 = ctx->regs[dest].host_temp;
                    append_load(&code, RTLTYPE_FLOAT32, host_src2,
                                X86_SP, -1, ctx->regs[src2].spill_offset);
                } else {
                    host_src2 = ctx->regs[src2].host_reg;
                }
                if (insn->elem == 0) {
                    /* PSLLQ instead of PSLLDQ to keep the high half of the
                     * register clear. */
                    append_insn_ModRM_reg(&code, false, X86OP_PSHIFTQ_U_I,
                                          X86OP_PSHIFT_SLL, host_dest);
                    append_imm8(&code, 32);
                    append_insn_ModRM_reg(
                        &code, false, X86OP_MOVSS_V_W, host_dest, host_src2);
                } else {
                    /* UNPCKLPS pushes the second element of the old vector
                     * into the high half of the register, so we need an
                     * extra MOVQ to clear it even if src2 was reloaded
                     * (and therefore has a zero second word). */
                    append_insn_ModRM_reg(&code, false, X86OP_UNPCKLPS,
                                          host_dest, host_src2);
                    append_insn_ModRM_reg(&code, false, X86OP_MOVQ_V_W,
                                          host_dest, host_dest);
                }
            } else {
                ASSERT(unit->regs[dest].type == RTLTYPE_V2_FLOAT64);
                if (insn->elem == 0) {
                    if (is_spilled(ctx, insn_index, src2)) {
                        append_insn_ModRM_mem(
                            &code, false, X86OP_MOVLPS_V_M, host_dest,
                            X86_SP, -1, ctx->regs[src2].spill_offset);
                    } else {
                        append_insn_ModRM_reg(
                            &code, false, X86OP_MOVSD_V_W, host_dest,
                            ctx->regs[src2].host_reg);
                    }
                } else {
                    append_insn_ModRM_ctx(&code, false, X86OP_MOVHPS_V_M,
                                          host_dest, ctx, insn_index, src2);
                }
            }
            break;
          }  // case RTLOP_VINSERT

          case RTLOP_VFCVT: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            if (unit->regs[dest].type == RTLTYPE_V2_FLOAT64) {
                ASSERT(unit->regs[src1].type == RTLTYPE_V2_FLOAT32);
                append_insn_ModRM_ctx(&code, false, X86OP_CVTPS2PD, host_dest,
                                      ctx, insn_index, src1);
            } else {
                ASSERT(unit->regs[dest].type == RTLTYPE_V2_FLOAT32);
                ASSERT(unit->regs[src1].type == RTLTYPE_V2_FLOAT64);
                append_insn_ModRM_ctx(&code, false, X86OP_CVTPD2PS, host_dest,
                                      ctx, insn_index, src1);
            }
            break;
          }  // case RTLOP_VFCVT

          case RTLOP_VFCMP: {
            // FIXME: This is mostly a hack to speed up vector NaN checking.
            // Need to implement other comparison types and/or find a better
            // way to do this.
            ASSERT(insn->fcmp == RTLFCMP_UN);
            const X86Register host_dest = ctx->regs[dest].host_reg;
            ASSERT(ctx->regs[dest].temp_allocated);
            const X86Register host_temp = ctx->regs[dest].host_temp;
            const bool is64 = (unit->regs[src1].type == RTLTYPE_V2_FLOAT64);
            const X86Opcode cmp_opcode = is64 ? X86OP_CMPPD : X86OP_CMPPS;

            append_move_or_load(&code, ctx, unit, insn_index, host_temp, src1);
            append_insn_ModRM_ctx(&code, false, cmp_opcode, host_temp,
                                  ctx, insn_index, src2);
            append_imm8(&code, X86XMMCMP_UNORD);
            if (is64) {
                append_insn_ModRM_reg(&code, false, X86OP_PSHIFTQ_U_I,
                                      X86OP_PSHIFT_SRLDQ, host_temp);
                append_imm8(&code, 4);
            }
            append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                  host_temp, host_dest);
            break;
          }  // case RTLOP_VFCMP

          case RTLOP_LOAD_IMM: {
            const uint64_t imm = insn->src_imm;
            const X86Register host_dest = ctx->regs[dest].host_reg;

            switch (unit->regs[dest].type) {
              case RTLTYPE_FLOAT32:
                if (imm == 0) {
                    append_insn_ModRM_reg(&code, false, X86OP_XORPS,
                                          host_dest, host_dest);
                } else {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    const X86Register host_temp = ctx->regs[dest].host_temp;
                    append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_temp);
                    append_imm32(&code, (uint32_t)imm);
                    append_insn_ModRM_reg(&code, false, X86OP_MOVD_V_E,
                                          host_dest, host_temp);
                }
                break;

              case RTLTYPE_FLOAT64:
                if (imm == 0) {
                    append_insn_ModRM_reg(&code, false, X86OP_XORPS,
                                          host_dest, host_dest);
                } else {
                    ASSERT(ctx->regs[dest].temp_allocated);
                    const X86Register host_temp = ctx->regs[dest].host_temp;
                    append_insn_R(&code, true, X86OP_MOV_rAX_Iv, host_temp);
                    append_imm64(&code, imm);
                    append_insn_ModRM_reg(&code, true, X86OP_MOVD_V_E,
                                          host_dest, host_temp);
                }
                break;

              default:
                ASSERT(rtl_type_is_int(unit->regs[dest].type));
                if (imm == 0) {
                    append_insn_ModRM_reg(&code, false, X86OP_XOR_Gv_Ev,
                                          host_dest, host_dest);
                    if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                        ctx->last_test_reg = dest;
                        ctx->last_cmp_reg = 0;
                    }
                } else {
                    append_load_imm_gpr(&code, host_dest, imm);
                }
                break;
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
                const bool is64 = int_type_is_64(unit->regs[dest].type);
                append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                      host_dest, host_src);
            }
            break;
          }  // case RTLOP_LOAD_ARG

          case RTLOP_LOAD: {
            const HostX86RegInfo * const dest_info = &ctx->regs[dest];
            const X86Register host_dest = dest_info->host_reg;
            const X86Register host_temp = (dest_info->temp_allocated
                                           ? dest_info->host_temp : host_dest);
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, host_temp,
                                  &host_base, &host_index);
            RTLDataType load_type = unit->regs[dest].type;
            if (load_type == RTLTYPE_V2_FLOAT32) {
                load_type = RTLTYPE_FLOAT64;  // Only load 8 bytes.
            }
            const int32_t offset =
                insn->host_data_32 ? (int32_t)insn->host_data_32 : insn->offset;
            append_load(&code, load_type, host_dest,
                        host_base, host_index, offset);
            break;
          }  // case RTLOP_LOAD

          case RTLOP_LOAD_U8:
          case RTLOP_LOAD_S8: {
            const HostX86RegInfo * const dest_info = &ctx->regs[dest];
            const X86Register host_dest = dest_info->host_reg;
            const X86Register host_temp = (dest_info->temp_allocated
                                           ? dest_info->host_temp : host_dest);
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, host_temp,
                                  &host_base, &host_index);
            const int32_t offset =
                insn->host_data_32 ? (int32_t)insn->host_data_32 : insn->offset;
            const X86Opcode opcode = (insn->opcode == RTLOP_LOAD_U8
                                      ? X86OP_MOVZX_Gv_Eb : X86OP_MOVSX_Gv_Eb);
            append_insn_ModRM_mem(&code, false, opcode, host_dest,
                                  host_base, host_index, offset);
            break;
          }  // case RTLOP_LOAD_U8, RTLOP_LOAD_S8

          case RTLOP_LOAD_U16:
          case RTLOP_LOAD_S16: {
            const HostX86RegInfo * const dest_info = &ctx->regs[dest];
            const X86Register host_dest = dest_info->host_reg;
            const X86Register host_temp = (dest_info->temp_allocated
                                           ? dest_info->host_temp : host_dest);
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, host_temp,
                                  &host_base, &host_index);
            const int32_t offset =
                insn->host_data_32 ? (int32_t)insn->host_data_32 : insn->offset;
            const X86Opcode opcode = (insn->opcode == RTLOP_LOAD_U16
                                      ? X86OP_MOVZX_Gv_Ew : X86OP_MOVSX_Gv_Ew);
            append_insn_ModRM_mem(&code, false, opcode, host_dest,
                                  host_base, host_index, offset);
            break;
          }  // case RTLOP_LOAD_U16, RTLOP_LOAD_S16

          case RTLOP_STORE: {
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, X86_R15,
                                  &host_base, &host_index);
            const int32_t offset = (int32_t)insn->host_data_32;
            const RTLRegister * const src2_reg = &unit->regs[src2];
            RTLDataType type = src2_reg->type;
            if (src2_reg->source == RTLREG_CONSTANT && !src2_reg->live) {
                /* Constant store optimized to memory-immediate operation. */
                const bool is64 = int_type_is_64(type);
                append_insn_ModRM_mem(&code, is64, X86OP_MOV_Ev_Iz, 0,
                                      host_base, host_index, offset);
                append_imm32(&code, (uint32_t)src2_reg->value.i64);
            } else {
                X86Register host_value = ctx->regs[src2].host_reg;
                if (is_spilled(ctx, insn_index, src2)) {
                    /* src3 is our value temporary (see register allocation).
                     * For plain stores, if we run out of GPRs we'll just use
                     * XMM15 instead, so there's no collision with the base
                     * or index register. */
                    host_value = insn->src3;
                    ASSERT(host_value != host_base
                           && (int)host_value != host_index);
                    if (host_value >= X86_XMM0 && rtl_type_is_int(type)) {
                        type = (int_type_is_64(type)
                                ? RTLTYPE_FLOAT64 : RTLTYPE_FLOAT32);
                    }
                    append_load(&code, type, host_value,
                                X86_SP, -1, ctx->regs[src2].spill_offset);
                }
                const RTLDataType store_type =
                    (type == RTLTYPE_V2_FLOAT32) ? RTLTYPE_FLOAT64 : type;
                append_store(&code, store_type, host_value,
                             host_base, host_index, offset);
            }
            break;
          }  // case RTLOP_STORE

          case RTLOP_STORE_I8: {
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, X86_R15,
                                  &host_base, &host_index);
            const int32_t offset = (int32_t)insn->host_data_32;
            const RTLRegister * const src2_reg = &unit->regs[src2];
            if (src2_reg->source == RTLREG_CONSTANT && !src2_reg->live) {
                /* Constant store optimized to memory-immediate operation. */
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Eb_Ib, 0,
                                      host_base, host_index, offset);
                append_imm8(&code, (uint8_t)src2_reg->value.i64);
            } else {
                X86Register host_value;
                const bool saved_ax = reload_store_source_gpr(
                    &code, ctx, insn_index, &host_base, &host_index,
                    &host_value);
                maybe_append_empty_rex(
                    &code, host_value, host_base, host_index);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Eb_Gb,
                                      host_value, host_base, host_index,
                                      offset);
                if (saved_ax) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                          X86_XMM15, X86_AX);
                }
            }
            break;
          }  // case RTLOP_STORE_I8

          case RTLOP_STORE_I16: {
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, X86_R15,
                                  &host_base, &host_index);
            const int32_t offset = (int32_t)insn->host_data_32;
            const RTLRegister * const src2_reg = &unit->regs[src2];
            if (src2_reg->source == RTLREG_CONSTANT && !src2_reg->live) {
                /* Constant store optimized to memory-immediate operation. */
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Iz, 0,
                                      host_base, host_index, offset);
                append_imm16(&code, (uint16_t)src2_reg->value.i64);
            } else {
                X86Register host_value;
                const bool saved_ax = reload_store_source_gpr(
                    &code, ctx, insn_index, &host_base, &host_index,
                    &host_value);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                      host_value, host_base, host_index,
                                      offset);
                if (saved_ax) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                          X86_XMM15, X86_AX);
                }
            }
            break;
          }  // case RTLOP_STORE_I16

          case RTLOP_LOAD_BR: {
            const HostX86RegInfo * const dest_info = &ctx->regs[dest];
            const X86Register host_dest = dest_info->host_reg;
            const X86Register host_temp = (dest_info->temp_allocated
                                           ? dest_info->host_temp : host_dest);
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, host_temp,
                                  &host_base, &host_index);
            const int32_t offset =
                insn->host_data_32 ? (int32_t)insn->host_data_32 : insn->offset;
            switch (unit->regs[dest].type) {
              case RTLTYPE_INT32:
              case RTLTYPE_INT64:
              case RTLTYPE_ADDRESS: {
                const bool is64 = int_type_is_64(unit->regs[dest].type);
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, is64, X86OP_MOVBE_Gy_My,
                                          host_dest, host_base, host_index,
                                          offset);
                } else {
                    append_insn_ModRM_mem(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_dest, host_base, host_index,
                                          offset);
                    append_insn_R(&code, is64, X86OP_BSWAP_rAX, host_dest);
                }
                break;
              }  // case RTLTYPE_{INT32,INT64,ADDRESS}
              case RTLTYPE_FLOAT32:
              case RTLTYPE_FLOAT64: {
                const bool is64 = (unit->regs[dest].type == RTLTYPE_FLOAT64);
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, is64, X86OP_MOVBE_Gy_My,
                                          host_temp, host_base, host_index,
                                          offset);
                } else {
                    append_insn_ModRM_mem(&code, is64, X86OP_MOV_Gv_Ev,
                                          host_temp, host_base, host_index,
                                          offset);
                    append_insn_R(&code, is64, X86OP_BSWAP_rAX, host_temp);
                }
                append_insn_ModRM_reg(&code, is64, X86OP_MOVD_V_E,
                                      host_dest, host_temp);
                break;
              }  // case RTLTYPE_{FLOAT32,FLOAT64}
              default:
                log_error(handle, "Invalid data type %s in LOAD_BR",
                          rtl_type_name(unit->regs[dest].type));
            }
            break;
          }  // case RTLOP_LOAD_BR

          case RTLOP_LOAD_U16_BR:
          case RTLOP_LOAD_S16_BR: {
            const HostX86RegInfo * const dest_info = &ctx->regs[dest];
            const X86Register host_dest = dest_info->host_reg;
            const X86Register host_temp = (dest_info->temp_allocated
                                           ? dest_info->host_temp : host_dest);
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, host_temp,
                                  &host_base, &host_index);
            const int32_t offset =
                insn->host_data_32 ? (int32_t)insn->host_data_32 : insn->offset;

            if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                append_insn_ModRM_mem(&code, false, X86OP_MOVBE_Gw_Mw,
                                      host_dest, host_base, host_index,
                                      offset);
            } else {
                /* MOVZX instead of plain MOV (which would leave the high
                 * bits of the destination register unchanged) to avoid a
                 * false dependency on the previous value of the register. */
                append_insn_ModRM_mem(&code, false, X86OP_MOVZX_Gv_Ew,
                                      host_dest, host_base, host_index,
                                      offset);
                /* rorw $8,%reg would be slightly more compact, but that
                 * incurs both a rotate penalty and a partial register
                 * stall when subsequently using the full 32-bit register.
                 * The byte-XCHG idiom (e.g. XCHG AH,AL) seems similarly
                 * likely to cause a partial register stall, and we could
                 * only use it with AX through DX anyway, so we don't do
                 * that either.  Modern processors should all support
                 * MOVBE anyway. */
                append_insn_R(&code, false, X86OP_BSWAP_rAX, host_dest);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_dest);
                append_imm8(&code, 16);
                if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                    ctx->last_test_reg = dest;
                    ctx->last_cmp_reg = 0;
                }
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
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, X86_R15,
                                  &host_base, &host_index);
            const int32_t offset = (int32_t)insn->host_data_32;
            X86Register host_value;
            const bool saved_ax = reload_store_source_gpr(
                &code, ctx, insn_index, &host_base, &host_index, &host_value);
            switch (unit->regs[src2].type) {
              case RTLTYPE_INT32:
              case RTLTYPE_INT64:
              case RTLTYPE_ADDRESS: {
                const bool is64 = int_type_is_64(unit->regs[src2].type);
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, is64, X86OP_MOVBE_My_Gy,
                                          host_value, host_base, host_index,
                                          offset);
                } else {
                    append_insn_R(&code, is64, X86OP_BSWAP_rAX, host_value);
                    append_insn_ModRM_mem(&code, is64, X86OP_MOV_Ev_Gv,
                                          host_value, host_base, host_index,
                                          offset);
                    if (!is_spilled(ctx, insn_index, src2)
                     && host_value == ctx->regs[src2].host_reg
                     && unit->regs[src2].death > insn_index) {
                        append_insn_R(&code, is64, X86OP_BSWAP_rAX,
                                      host_value);
                    }
                }
                break;
              }  // case RTLTYPE_{INT32,INT64,ADDRESS}
              case RTLTYPE_FLOAT32:
              case RTLTYPE_FLOAT64: {
                const bool is64 = (unit->regs[src2].type == RTLTYPE_FLOAT64);
                if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                    append_insn_ModRM_mem(&code, is64, X86OP_MOVBE_My_Gy,
                                          host_value, host_base, host_index,
                                          offset);
                } else {
                    append_insn_R(&code, is64, X86OP_BSWAP_rAX, host_value);
                    append_insn_ModRM_mem(&code, is64, X86OP_MOV_Ev_Gv,
                                          host_value, host_base, host_index,
                                          offset);
                }
                break;
              }  // case RTLTYPE_{FLOAT32,FLOAT64}
              default:
                log_error(handle, "Invalid data type %s in STORE_BR",
                          rtl_type_name(unit->regs[src2].type));
            }
            if (saved_ax) {
                append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                      X86_XMM15, X86_AX);
            }
            break;
          }  // case RTLOP_STORE_BR

          case RTLOP_STORE_I16_BR: {
            X86Register host_base;
            int host_index;
            reload_base_and_index(&code, ctx, insn_index, X86_R15,
                                  &host_base, &host_index);
            const int32_t offset = (int32_t)insn->host_data_32;
            X86Register host_value;
            const bool saved_ax = reload_store_source_gpr(
                &code, ctx, insn_index, &host_base, &host_index, &host_value);
            if (handle->setup.host_features & BINREC_FEATURE_X86_MOVBE) {
                append_insn_ModRM_mem(&code, false, X86OP_MOVBE_Mw_Gw,
                                      host_value, host_base, host_index,
                                      offset);
            } else if (is_spilled(ctx, insn_index, src2)
                       || host_value != ctx->regs[src2].host_reg
                       || unit->regs[src2].death <= insn_index) {
                append_insn_R(&code, false, X86OP_BSWAP_rAX, host_value);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_SHR, host_value);
                append_imm8(&code, 16);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                      host_value, host_base, host_index,
                                      offset);
                /* We can't treat this as a test of the register because
                 * there might be data in the high 16 bits. */
                ctx->last_test_reg = 0;
                ctx->last_cmp_reg = 0;
            } else {
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_ROR, host_value);
                append_imm8(&code, 8);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_mem(&code, false, X86OP_MOV_Ev_Gv,
                                      host_value, host_base, host_index,
                                      offset);
                append_opcode(&code, X86OP_OPERAND_SIZE);
                append_insn_ModRM_reg(&code, false, X86OP_SHIFT_Ev_Ib,
                                      X86OP_SHIFT_ROR, host_value);
                append_imm8(&code, 8);
                ctx->last_test_reg = 0;
                ctx->last_cmp_reg = 0;
            }
            if (saved_ax) {
                append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                      X86_XMM15, X86_AX);
            }
            break;
          }  // case RTLOP_STORE_I16_BR

          case RTLOP_ATOMIC_INC: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            X86Register host_base;
            int host_index;
            /* A temporary register is only allocated if there are any
             * spill reloads, but if there are no reloads then the fallback
             * register isn't used anyway, so it's safe to pass host_temp
             * without checking temp_allocated. */
            reload_base_and_index(&code, ctx, insn_index,
                                  ctx->regs[dest].host_temp,
                                  &host_base, &host_index);
            const bool is64 = int_type_is_64(unit->regs[dest].type);
            append_insn_R(&code, false, X86OP_MOV_rAX_Iv, host_dest);
            append_imm32(&code, 1);
            append_opcode(&code, X86OP_LOCK);
            append_insn_ModRM_mem(
                &code, is64, X86OP_XADD_Ev_Gv, host_dest,
                host_base, host_index, insn->host_data_32);
            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                ctx->last_test_reg = dest;
                ctx->last_cmp_reg = 0;
            }
            break;
          }  // case RTLOP_ATOMIC_INC

          case RTLOP_CMPXCHG: {
            const X86Register host_dest = ctx->regs[dest].host_reg;
            ASSERT(host_dest != X86_AX);
            X86Register host_src1 = ctx->regs[src1].host_reg;
            X86Register host_src3 = ctx->regs[insn->src3].host_reg;
            X86Register host_temp;
            int temp_index = 0;
            const bool is64 = int_type_is_64(unit->regs[dest].type);

            /* If we have a temporary, we need to save RAX.  However, we
             * want to save the allocated temporary (a GPR) for a CMPXCHG
             * operand, so we save RAX to R15, or to XMM15 if R15 was
             * allocated as the temporary. */
            if (ctx->regs[dest].temp_allocated) {
                if (ctx->regs[dest].host_temp == X86_R15) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOVD_V_E,
                                          X86_XMM15, X86_AX);
                } else {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          X86_R15, X86_AX);
                }
                host_temp = ctx->regs[dest].host_temp;
            } else {
                /* RAX is free (or already in use by an operand), so our
                 * temporary is R15. */
                host_temp = X86_R15;
            }

            /* Reload src1 and src3, if needed. */
            if (is_spilled(ctx, insn_index, src1)) {
                append_load_gpr(&code, RTLTYPE_ADDRESS, host_temp,
                                X86_SP, ctx->regs[src1].spill_offset);
                host_src1 = host_temp;
                host_temp = host_dest;
                /* Make sure we're not about to overwrite src2 in case src3
                 * is also spilled (the register allocator guarantees this). */
                ASSERT(!(!is_spilled(ctx, insn_index, src2)
                         && host_dest == ctx->regs[src2].host_reg));
                temp_index++;
            }
            if (is_spilled(ctx, insn_index, insn->src3)) {
                append_load_gpr(&code, unit->regs[insn->src3].type, host_temp,
                                X86_SP, ctx->regs[insn->src3].spill_offset);
                host_src3 = host_temp;
                temp_index++;
            }

            /* If we have an index register and it's spilled, "reload" it
             * by adding it to the address register and subtracting it
             * again afterward.  This is obviously slow, but it should
             * probably be uncommon since address generation for atomic
             * operations is normally fairly localized. */
            int host_index = -1;
            bool index_spilled = false;
            if (insn->host_data_16) {
                HostX86RegInfo *index_info = &ctx->regs[insn->host_data_16];
                if (is_spilled(ctx, insn_index, insn->host_data_16)) {
                    log_warning(handle, "Slow reload of spilled CMPXCHG"
                                " index register");
                    index_spilled = true;
                    ASSERT(unit->regs[insn->host_data_16].type
                           == RTLTYPE_ADDRESS);
                    append_insn_ModRM_mem(
                        &code, true, X86OP_ADD_Gv_Ev, host_src1,
                        X86_SP, -1, index_info->spill_offset);
                } else {
                    host_index = index_info->host_reg;
                }
            }

            /* If we have an index register in RAX and both src1 and src3
             * were spilled, add the index to the reloaded src1 so it's not
             * in the way of src2.  Since host_src1 is a temporary in this
             * case, we don't have to worry about restoring its old value
             * later.  We don't try to use the rAX save register because
             * (1) it won't exist if the index dies on this instruction
             * and (2) we can't use it as an index if it's XMM15. */
            if (host_index == X86_AX && temp_index == 2) {
                ASSERT(is_spilled(ctx, insn_index, src1));
                append_insn_ModRM_reg(&code, true, X86OP_ADD_Gv_Ev,
                                      host_src1, host_index);
                host_index = -1;
            }

            /* Load src2 (the compare value) into rAX.  If any other
             * operand is in rAX, save it in the destination register;
             * note that we don't need to restore it from dest later,
             * since if it's live past this instruction, it will already
             * have been saved in (and be restored from) R15 or XMM15.
             * We also don't have to worry about clobbering anything
             * that's already in dest, since the register allocator avoids
             * reusing the register of any unspilled input operand. */
            if (host_src1 == X86_AX || host_src3 == X86_AX
             || host_index == X86_AX) {
                ASSERT(temp_index < 2);
                /* If we saved RAX to R15 above, this MOV is technically
                 * unnecessary, but the logic to use R15 in that specific
                 * case (which will probably be fairly rare) is more
                 * complex than it's worth, especially since MOVs are
                 * potentially zero-latency.  Likewise, we don't try to
                 * omit this MOV if src2 is also in rAX (due to being the
                 * same RTL register as src1 or src3). */
                const bool is64_ax = (is64 || host_src1 == X86_AX
                                           || host_index == X86_AX);
                append_insn_ModRM_reg(&code, is64_ax, X86OP_MOV_Gv_Ev,
                                      host_dest, X86_AX);
                if (host_src1 == X86_AX) {
                    host_src1 = host_dest;
                }
                if (host_src3 == X86_AX) {
                    host_src3 = host_dest;
                }
                if (host_index == X86_AX) {
                    host_index = host_dest;
                }
            }
            append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                    X86_AX, src2);

            /* Do the actual compare-and-swap. */
            append_opcode(&code, X86OP_LOCK);
            append_insn_ModRM_mem(
                &code, is64, X86OP_CMPXCHG_Ev_Gv, host_src3,
                host_src1, host_index, insn->host_data_32);

            /* Undo the ADD from index reloading if necessary. */
            if (index_spilled) {
                append_insn_ModRM_mem(
                    &code, true, X86OP_SUB_Gv_Ev, host_src1,
                    X86_SP, -1, ctx->regs[insn->host_data_16].spill_offset);
            }

            /* Move the result to the destination.  The instruction
             * description says that the value of the compare target is
             * only written to the result register (rAX) if the compare
             * fails, but if the compare succeeds, rAX already has the
             * correct value, so we can use it unconditionally. */
            append_insn_ModRM_reg(&code, is64, X86OP_MOV_Gv_Ev,
                                  host_dest, X86_AX);

            /* Restore RAX if necessary. */
            if (ctx->regs[dest].temp_allocated) {
                if (ctx->regs[dest].host_temp == X86_R15) {
                    append_insn_ModRM_reg(&code, true, X86OP_MOVD_E_V,
                                          X86_XMM15, X86_AX);
                } else {
                    append_insn_ModRM_reg(&code, true, X86OP_MOV_Gv_Ev,
                                          X86_AX, X86_R15);
                }
            }

            if (handle->host_opt & BINREC_OPT_H_X86_CONDITION_CODES) {
                ctx->last_test_reg = 0;
                ctx->last_cmp_reg = dest;
                ctx->last_cmp_target = src2;
                ctx->last_cmp_imm = 0;
            }
            break;
          }  // case RTLOP_CMPXCHG

          case RTLOP_LABEL:
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(ctx->label_offsets[insn->label] < 0);

            if (handle->host_opt & BINREC_OPT_H_X86_BRANCH_ALIGNMENT) {
                /*
                 * Intel's documentation recommends aligning all branch
                 * targets to a multiple of 16 byte so that the instruction
                 * decoder (which fetches aligned 16-byte blocks) can read
                 * as many instructions as possible.  However, we have to
                 * balance that with the fact that all NOPs which appear
                 * in the actual code path have to be decoded and executed
                 * just like other instructions.  So we use the following
                 * heuristic to decide whether to align a label:
                 *
                 * - If the label follows an unconditional branch, so that
                 *   execution never falls into the block, always align
                 *   the label since there's no penalty (other than
                 *   increased code size) for doing so.
                 *
                 * - Otherwise, if the label is the target of a backward
                 *   branch, align it if there are less than 10 bytes left
                 *   in the current 16-byte line, since backward branches
                 *   generally indicate loops and we can thus expect them
                 *   to be reached more often by branching than by falling
                 *   through.
                 *
                 * - Otherwise, only align the label if there are less
                 *   than 5 bytes left in the current 16-byte line.
                 */

                bool follows_uncond = false;
                if (block->prev_block >= 0) {
                    const RTLBlock *prev_block =
                        &unit->blocks[block->prev_block];
                    const RTLInsn *prev_insn =
                        &unit->insns[prev_block->last_insn];
                    follows_uncond = (prev_insn->opcode == RTLOP_GOTO
                                   || prev_insn->opcode == RTLOP_RETURN);
                }
                const int align_distance = (16 - code.len) & 15;

                bool should_align;
                if (follows_uncond) {
                    should_align = true;
                } else if (insn->host_data_16) {
                    /* host_data_16 is set to nonzero if the label is
                     * targeted by a backward branch. */
                    should_align = (align_distance < 10);
                } else {
                    should_align = (align_distance < 5);
                }

                if (should_align) {
                    append_nops(&code, align_distance);
                    ASSERT((code.len & 15) == 0);
                }
            }

            ctx->label_offsets[insn->label] = code.len;
            break;

          case RTLOP_GOTO:
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(block_info->unresolved_branch_offset < 0);
            ASSERT(unit->label_blockmap[insn->label] >= 0);
            if (!reload_regs_for_block(&code, ctx, block_index,
                                       unit->label_blockmap[insn->label])) {
                return false;
            }
            initial_len = code.len;  // Don't include setup in length check.
            append_jump(&code, block_info, X86OP_JMP_Jb, X86OP_JMP_Jz,
                        insn->label, ctx->label_offsets[insn->label]);
            fall_through = false;
            break;

          case RTLOP_GOTO_IF_Z:
          case RTLOP_GOTO_IF_NZ: {
            ASSERT(insn->label > 0);
            ASSERT(insn->label < unit->next_label);
            ASSERT(block_info->unresolved_branch_offset < 0);
            const int target_block = unit->label_blockmap[insn->label];
            ASSERT(target_block >= 0);

            uint8_t jump_condition;
            if (insn->host_data_16) {
                jump_condition = insn->host_data_16 & 0xF;
            } else {
                jump_condition = (insn->opcode == RTLOP_GOTO_IF_Z
                                  ? X86CC_Z : X86CC_NZ);
            }
            const X86Opcode short_opcode = X86OP_Jcc_Jb | jump_condition;
            const X86Opcode long_opcode = X86OP_Jcc_Jz | jump_condition;

            if (insn->host_data_16 & 0x40) {  // FTESTEXC
                ASSERT(insn->host_data_16 & 0x10);
                if (!is_spilled(ctx, insn_index, src1)) {
                    maybe_append_empty_rex(
                        &code, ctx->regs[src1].host_reg, -1, -1);
                }
                append_insn_ModRM_ctx(
                    &code, false, X86OP_UNARY_Eb, X86OP_UNARY_TEST,
                    ctx, insn_index, src1);
                append_imm8(&code, rtlfexc_to_bits(insn->host_data_32));
                ctx->last_test_reg = 0;
                ctx->last_cmp_reg = 0;
            } else {
                append_compare(ctx, insn_index, &code, src1, src2,
                               insn->host_data_32,
                               (insn->host_data_16 >> 8) & 0x1F,
                               (jump_condition & 0xE) == X86CC_Z,
                               (insn->host_data_16 & 0x20) != 0, -1);
            }

            /* If we have any aliases or spills to reload that would
             * conflict with live registers, we have to invert the sense of
             * the branch here and set up the registers conditionally. */
            if (check_reload_conflicts(ctx, block_index, insn_index)) {
                uint8_t reload_buffer[RELOAD_REGS_SIZE];
                CodeBuffer reload_code = {.buffer = reload_buffer,
                                         .buffer_size = sizeof(reload_buffer),
                                         .len = 0};
                ASSERT(reload_regs_for_block(&reload_code, ctx, block_index,
                                             target_block));
                /* Write this jump as though the next one (to the target
                 * label) will have a 32-bit displacement.  If it ends up
                 * having an 8-bit displacement, we'll fix up this
                 * instruction afterward. */
                const long reload_jump = code.len;
                /* Flipping the low bit of the opcode will invert the sense
                 * of the branch. */
                const int jump_disp = reload_code.len + 5;
                const int jump_opcode = (jump_disp < 128
                                         ? short_opcode ^ 1 : long_opcode ^ 1);
                append_jump_raw(&code, jump_opcode, jump_disp);
                const long reload_start = code.len;
                const long needed_space = reload_code.len + 5;
                if (UNLIKELY(code.len + needed_space > code.buffer_size)) {
                    handle->code_len = code.len;
                    if (UNLIKELY(!binrec_ensure_code_space(
                                     handle, needed_space))) {
                        log_error(handle, "No memory for alias conflict"
                                  " resolution code");
                        return false;
                    }
                    code.buffer = handle->code_buffer;
                    code.buffer_size = handle->code_buffer_size;
                }
                memcpy(&code.buffer[code.len], reload_code.buffer,
                       reload_code.len);
                code.len += reload_code.len;
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
                    ASSERT(reload_start == reload_jump + 2);
                    code.buffer[reload_start - 1] -= 3;
                }
                initial_len = code.len;  // Suppress output length check.
            } else {
                if (!reload_regs_for_block(&code, ctx, block_index,
                                           target_block)) {
                    return false;
                }
                initial_len = code.len; // Don't include setup in length check.
                append_jump(&code, block_info, short_opcode, long_opcode,
                            insn->label, ctx->label_offsets[insn->label]);
            }
            break;
          }  // case RTLOP_GOTO_IF_Z, RTLOP_GOTO_IF_NZ

          case RTLOP_CALL:
          case RTLOP_CALL_TRANSPARENT:
            handle->code_len = code.len;
            if (!translate_call(ctx, block_index, insn_index)) {
                return false;
            }
            code.buffer = handle->code_buffer;
            code.buffer_size = handle->code_buffer_size;
            code.len = handle->code_len;
            initial_len = code.len;  // Suppress output length check.
            break;

          case RTLOP_RETURN:
            ASSERT(block_info->unresolved_branch_offset < 0);
            if (src1) {
                append_move_or_load_gpr(&code, ctx, unit, insn_index,
                                        X86_AX, src1);
            }
            /* If this instruction terminates the last block in the unit,
             * we don't need an explicit jump to the epilogue. */
            ASSERT(insn_index == block->last_insn);
            if (block->next_block >= 0) {
                /* We use label 0 (normally invalid) to indicate a jump to
                 * the function epilogue. */
                append_jump(&code, block_info,
                            X86OP_JMP_Jb, X86OP_JMP_Jz, 0, -1);
            }
            fall_through = false;
            break;

          case RTLOP_CHAIN:
            handle->code_len = code.len;
            if (!translate_chain(ctx, insn_index)) {
                return false;
            }
            code.buffer = handle->code_buffer;
            code.buffer_size = handle->code_buffer_size;
            code.len = handle->code_len;
            initial_len = code.len;  // Suppress output length check.
            break;

          case RTLOP_CHAIN_RESOLVE:
            translate_chain_resolve(ctx, &code, insn_index);
            break;

          case RTLOP_ILLEGAL:
            append_opcode(&code, X86OP_UD2);
            break;

        }  // switch (insn->opcode)

        ASSERT(code.len - initial_len <= MAX_INSN_LEN);
    }

    if (fall_through && block->next_block >= 0) {
        if (!reload_regs_for_block(&code, ctx, block_index,
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
 * resolve_branches:  Resolve forward branches in the generated code.
 *
 * When generating a forward branch, we don't yet know what the offset to
 * the target instruction will be, so we use this function to fill it in
 * after code generation is complete.
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
            int64_t offset = ctx->label_offsets[label] - branch_offset;
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
    ASSERT(ctx->unit);

    const RTLUnit * const unit = ctx->unit;

    /* Push all stack frame offsets forward by the callee reserve amount
     * (if any) so the low-level translation logic doesn't have to worry
     * about it. */
    if (ctx->frame_callee_reserve > 0) {
        for (int reg_index = 1; reg_index < unit->next_reg; reg_index++) {
            if (ctx->regs[reg_index].spilled) {
                ctx->regs[reg_index].spill_offset += ctx->frame_callee_reserve;
            }
        }
        for (int reg = 0; reg < 32; reg++) {
            if (ctx->stack_callsave[reg] >= 0) {
                ctx->stack_callsave[reg] += ctx->frame_callee_reserve;
            }
        }
        /* Currently, we always allocate a frame slot for MXCSR when
         * translating a call-type instruction. */
        ASSERT(ctx->stack_mxcsr >= 0);
        ctx->stack_mxcsr += ctx->frame_callee_reserve;
    }

    if (!append_prologue(ctx)) {
        return false;
    }

    memset(ctx->reg_map, 0, sizeof(ctx->reg_map));
    for (int i = 0; i >= 0; i = unit->blocks[i].next_block) {
        if (!translate_block(ctx, i)) {
            return false;
        }
    }

    if (!append_epilogue(ctx, true)) {
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
    memset(ctx->stack_callsave, -1, sizeof(ctx->stack_callsave));
    ctx->stack_mxcsr = -1;

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
