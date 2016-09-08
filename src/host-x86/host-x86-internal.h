/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINREC_HOST_X86_INTERNAL_H
#define BINREC_HOST_X86_INTERNAL_H

#include "src/common.h"
#include "src/rtl.h"

/*************************************************************************/
/**************** Internal constants and data structures *****************/
/*************************************************************************/

/* Constants identifying x86 registers.  These are defined such that the
 * low 3 bits of the each value match the register's index in various
 * opcode bytes. */
typedef enum X86Register {
    X86_AX    = 0,
    X86_CX    = 1,
    X86_DX    = 2,
    X86_BX    = 3,
    X86_SP    = 4,
    X86_BP    = 5,
    X86_SI    = 6,
    X86_DI    = 7,
    X86_R8    = 8,
    X86_R9    = 9,
    X86_R10   = 10,
    X86_R11   = 11,
    X86_R12   = 12,
    X86_R13   = 13,
    X86_R14   = 14,
    X86_R15   = 15,
    X86_XMM0  = 16,
    X86_XMM1  = 17,
    X86_XMM2  = 18,
    X86_XMM3  = 19,
    X86_XMM4  = 20,
    X86_XMM5  = 21,
    X86_XMM6  = 22,
    X86_XMM7  = 23,
    X86_XMM8  = 24,
    X86_XMM9  = 25,
    X86_XMM10 = 26,
    X86_XMM11 = 27,
    X86_XMM12 = 28,
    X86_XMM13 = 29,
    X86_XMM14 = 30,
    X86_XMM15 = 31,
} X86Register;

/* Constants for the 4 variable bits in a REX prefix. */
enum {
    X86_REX_W = 8,
    X86_REX_R = 4,
    X86_REX_X = 2,
    X86_REX_B = 1,
};

/*-----------------------------------------------------------------------*/

/* Data associated with each RTL register. */
typedef struct HostX86RegInfo {
    /* Has a host register been allocated for this register? */
    uint8_t host_allocated;
    /* Index of allocated register (X86Register). */
    uint8_t host_reg;
    /* Has a stack frame slot been allocated to spill this register? */
    uint8_t frame_allocated;
    /* Index of the allocated stack frame. */
    uint8_t frame_slot;
    /* Byte offset (from SP) of the allocated stack frame. */
    int16_t stack_offset;
    /* Next register in the merge chain, or 0 if the end of the chain. */
    uint32_t next_merged;
} HostX86RegInfo;

/* Data associated with each basic block. */
typedef struct HostX86BlockInfo {
    /* Map from host registers to RTL registers as of the beginning of the
     * block.  Set during register allocation and used to initialize the
     * current register map when starting to translate the block. */
    uint32_t initial_reg_map[32];
    /* Code buffer offset of an unresolved branch instruction at the end of
     * this block, or -1 if the block does not have an unresolved branch. */
    long unresolved_branch_offset;
} HostX86BlockInfo;

/* Context block used to maintain translation state. */
typedef struct HostX86Context {
    /* Arguments passed from binrec_translate(). */
    binrec_t *handle;
    RTLUnit *unit;

    /* Bitmap of callee-saved registers for the selected ABI. */
    uint32_t callee_saved_regs;

    /* Information for each basic block. */
    HostX86BlockInfo *blocks;
    /* Information for each RTL register. */
    HostX86RegInfo *regs;
    /* Generated code offsets for each label, and to the epilogue (which
     * is handled internally as label 0). */
    long *label_offsets;

    /* Current mapping from x86 to RTL registers. */
    uint32_t reg_map[32];
    /* Current bitmap of available registers. */
    uint32_t regs_free;
    /* Bitmap of registers which are used at least once (for saving and
     * restoring registers in the prologue/epilogue). */
    uint32_t regs_touched;
    /* RTL register whose value was used to set the condition codes, or 0
     * if the flags do not represent the value of a particular register. */
    uint32_t cc_reg;

    /* Stack frame size.  Must be a multiple of 16. */
    int frame_size;
    /* Total stack allocation, excluding PUSH instructions. */
    int stack_alloc;
} HostX86Context;

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

/**
 * host_x86_allocate_registers:  Allocate a host register for each RTL
 * register which needs one.
 *
 * On return, ctx->callee_saved_regs will be set to the proper value for
 * the selected ABI.
 *
 * [Parameters]
 *     ctx: Translation context.
 */
#define host_x86_allocate_registers INTERNAL(host_x86_allocate_registers)
extern void host_x86_allocate_registers(HostX86Context *ctx);

/**
 * host_x86_int_arg_register:  Return the register used for the given
 * integer function argument.  The argument index must be within the
 * range of arguments passed in registers for the selected host ABI
 * (0-5 for x86-64 SysV, 0-3 for x86-64 Windows).
 *
 * [Parameters]
 *     ctx: Translation context.
 *     index: Argument index.
 * [Return value]
 *     Register for argument.
 */
#define host_x86_int_arg_register INTERNAL(host_x86_int_arg_register)
extern PURE_FUNCTION X86Register host_x86_int_arg_register(
    HostX86Context *ctx, int index);

/*************************************************************************/
/*************************************************************************/

#endif  // BINREC_HOST_X86_INTERNAL_H
