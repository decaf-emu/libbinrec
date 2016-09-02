/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef SRC_RTL_INTERNAL_H
#define SRC_RTL_INTERNAL_H

/*************************************************************************/
/************************* Configuration options *************************/
/*************************************************************************/

/*============ General options ============*/

/**
 * INSNS_EXPAND_SIZE:  Specifies the number of instructions by which to
 * expand a block's instruction array when the array is full.  This value
 * is also used for the initial size of the array.
 */
#define INSNS_EXPAND_SIZE  1000

/**
 * BLOCKS_EXPAND_SIZE:  Specifies the number of blocks by which to expand a
 * block's basic block array when the array is full.  This value is also
 * used for the initial size of the array.
 */
#define BLOCKS_EXPAND_SIZE  100

/**
 * REGS_EXPAND_SIZE:  Specifies the number of register entries by which to
 * expand a block's register array when the array is full.  This value is
 * also used for the initial size of the array.
 */
#define REGS_EXPAND_SIZE  1000

/**
 * REGS_LIMIT:  Specifies the maximum number of registers allowed for a
 * single block.  Must be no greater than 65535 (because this value must
 * fit into a uint16_t).  The actual number of available registers is one
 * less than this value, since register 0 is never used.
 */
#define REGS_LIMIT  65535

/**
 * ALIASES_EXPAND_SIZE:  Specifies the number of entries by which to expand
 * a block's alias register array when the array is full.  This value is
 * also used for the initial size of the array.
 */
#define ALIASES_EXPAND_SIZE  100

/**
 * ALIASES_LIMIT:  Specifies the maximum number of alias registers allowed
 * for a single block.  Must be no greater than 65535 (because this value
 * must fit into a uint16_t).  The actual number of available alias
 * registers is one less than this value, since register 0 is never used.
 */
#define ALIASES_LIMIT  65535

/**
 * LABELS_EXPAND_SIZE:  Specifies the number of entries by which to expand
 * a block's label-to-block mapping array when the array is full.  This
 * value is also used for the initial size of the array.
 */
#define LABELS_EXPAND_SIZE  100

/**
 * LABELS_LIMIT:  Specifies the maximum number of labels allowed for a
 * single block.  Must be no greater than 65535 (because this value must
 * fit into a uint16_t).  The actual number of available labels is one less
 * than this value, since label 0 is never used.
 */
#define LABELS_LIMIT  65535

/**
 * NATIVE_EXPAND_SIZE:  Specifies the block size (in bytes) by which to
 * expand the native code buffer as necessary when translating.
 */
#define NATIVE_EXPAND_SIZE 8192

/*************************************************************************/
/*************************** Type declarations ***************************/
/*************************************************************************/

#undef mips  // Avoid namespace pollution from the compiler on MIPS machines

/*-----------------------------------------------------------------------*/

/**
 * RTLInsn:  A single platform-neutral (more or less) operation.  SH-2
 * instructions are translated into sequences of RTLInsns, which are then
 * optimized and retranslated into MIPS instructions.
 */
typedef struct RTLInsn_ {
    uint8_t opcode;           // Operation code (RTLOpcode)
    uint16_t dest;            // Destination register
    uint16_t src1, src2;      // Source registers
    union {
        uint16_t cond;        // Condition register for SELECT
        struct {
            uint8_t start;    // First (lowest) bit number for a bitfield
            uint8_t count;    // Number of bits for a bitfield
        } bitfield;
        int16_t offset;       // Byte offset for load/store instructions
        uint16_t label;       // GOTO target label
        uint16_t target;      // CALL_NATIVE branch target register
        uint64_t src_imm;     // Source immediate value
    };
} RTLInsn;

/*----------------------------------*/

/**
 * RTLRegType:  The type (source information) of a register used in an RTL
 * unit.
 */
typedef enum RTLRegType_ {
    RTLREG_UNDEFINED = 0,   // Not yet defined to anything
    RTLREG_CONSTANT,        // Constant value
    RTLREG_FUNC_ARG,        // Function argument
    RTLREG_MEMORY,          // Memory reference
    RTLREG_ALIAS,           // Loaded from an alias register
    RTLREG_RESULT,          // Result of an operation on other registers
    RTLREG_RESULT_NOFOLD,   // Result of an operation (not constant foldable)
} RTLRegType;

/**
 * RTLRegister:  Data about registers used in an RTL unit.
 */
typedef struct RTLRegister RTLRegister;
struct RTLRegister {
    /* Basic register information. */
    uint8_t type;               // Register data type (RTLDataType)
    uint8_t source;             // Register source (RTLRegType)

    /* Unique pointer information.  The "unique_pointer" field has the
     * property that all registers with the same nonzero value for
     * "unique_pointer" are native addresses which point to the same region
     * of memory, and that region of memory will only be accessed through
     * a register with the same "unique_pointer" value.  The field is set
     * initially by a call to rtl_make_unique_pointer(), and that value is
     * carried forward to other registers via data flow analysis. */
    uint16_t unique_pointer;

    /* Liveness information */
    uint8_t live;               // Nonzero if this register has been referenced
                                //    (this field is never cleared once set)
    uint16_t live_link;         // Next register in live list (sorted by birth)
    uint32_t birth;             // First RTL insn index when register is live
                                //    (= insn where it's assigned, because SSA)
    uint32_t death;             // Last RTL insn index when register is live

    /* Register value information. */
    union {
        union {                 // Value of register for RTLREG_CONSTANT
            uint32_t int32;
            uint64_t address;
            float float_;
            double double_;
        } value;
        unsigned int arg_index; // Function argument index for RTLREG_FUNC_ARG
        struct {
            uint16_t addr_reg;  // Register holding address for RTLREG_MEMORY
            int16_t offset;     // Access offset
            uint8_t size;       // Access size in bytes if this is an INT32
                                //    register (1, 2, or 4); otherwise unused
            uint8_t is_signed;  // Nonzero if a signed load for INT32, zero
                                //    if unsigned
        } memory;
        struct {
            uint16_t src;       // Source alias register
        } alias;
        struct {
            uint8_t opcode;     // Operation code for RTLREG_RESULT
            uint16_t src1;      // Operand 1
            uint16_t src2;      // Operand 2
            union {
                uint16_t cond;  // Condition register for SELECT
                struct {
                    uint8_t start;  // Start bit for bitfields
                    uint8_t count;  // Bit count for bitfields
                };
            };
        } result;
    };

    /* The following fields are for use by RTL-to-native translators: */
    uint32_t last_used;         // Last insn index where this register was used
    uint8_t native_allocated;   // Nonzero if a native reg has been allocated
    uint8_t native_reg;         // Native register allocated for this register
    uint8_t frame_allocated;    // Nonzero if a frame slot has been allocated
    uint8_t frame_slot;         // Frame slot allocated for this register
    int16_t stack_offset;       // Stack offset of this register's frame slot
    RTLRegister *next_merged;   // Next register in merge chain, or NULL
    union {
        struct {
            uint8_t something;  // FIXME: not yet implemented
        } x86;
    };
};

/*----------------------------------*/

/**
 * RTLBlock:  Information about an basic block of code (a sequence of
 * instructions with one entry point and one exit point).  Note that a block
 * can be empty, denoted by last_insn < first_insn, and that last_insn can
 * be negative, if first_insn is 0 and the block is empty.
 */
typedef struct RTLBlock {
    int32_t first_insn;         // unit->insns[] index of first insn in block
    int32_t last_insn;          // unit->insns[] index of last insn in block
    int16_t next_block;         // unit->blocks[] index of next block in code
                                //    stream (excluding dropped blocks);
                                //    -1 indicates the end of the code stream
    int16_t prev_block;         // unit->blocks[] index of previous block in
                                //    code stream, or -1 if the first block
    int16_t entries[8];         // unit->blocks[] indices of dominating blocks;
                                //    -1 indicates an unused slot.  Holes in
                                //    the list are not permitted.  For more
                                //    than 8 slots, add a dummy block on top.
                                //    (rtl_block_*() functions handle all this)
    int16_t exits[2];           // unit->blocks[] indices of postdominating
                                //    blocks.  A terminating insn can go at
                                //    most two places (conditional GOTO).

    /* The following fields are used only by RTL-to-native translators: */
    union {
        struct {
            uint8_t something;  // FIXME: not yet implemented
        } x86;
    };
} RTLBlock;

/*----------------------------------*/

/**
 * RTLUnit:  State information used in translating a unit of code.  The
 * RTLUnit type itself is defined in rtl.h.
 */
struct RTLUnit {
    binrec_t *handle;           // Translation handle

    RTLInsn *insns;             // Instruction array
    uint32_t insns_size;        // Size of instruction array (entries)
    uint32_t num_insns;         // Number of instructions actually in array

    RTLBlock *blocks;           // Basic block array
    uint16_t blocks_size;       // Size of block array (entries)
    uint16_t num_blocks;        // Number of blocks actually in array
    uint8_t have_block;         // Nonzero if there is a currently active block
    uint16_t cur_block;         // Current block index if have_block != 0

    int16_t *label_blockmap;    // Label-to-block-index mapping (-1 = unset)
    uint16_t labels_size;       // Size of label-to-block map array (entries)
    uint16_t next_label;        // Next label number to allocate

    RTLRegister *regs;          // Register array
    uint16_t regs_size;         // Size of register array (entries)
    uint16_t next_reg;          // Next register number to allocate
    uint16_t first_live_reg;    // First register in live range list
    uint16_t last_live_reg;     // Last register in live range list

    RTLDataType *alias_types;   // Alias register data type array
    uint16_t aliases_size;      // Size of alias register array (entries)
    uint16_t next_alias;        // Next alias register number to allocate

    uint8_t finalized;          // Nonzero if unit has been finalized

    /* The following fields are used only by optimization routines: */
    uint8_t *block_seen;        // Array of "seen" flags for all blocks
                                //    (used by rtlopt_drop_dead_blocks())

    /* The following fields are used only by RTL-to-native translators: */
    void *native_buffer;        // Native code buffer
    uint32_t native_bufsize;    // Allocated size of native code buffer
    uint32_t native_length;     // Length of native code
    uint32_t *label_offsets;    // Array of native offsets for labels
    union {
        struct {
            uint8_t something;  // FIXME: not yet implemented
        } x86;
    };
};

/*************************************************************************/
/**************** Library-internal function declarations *****************/
/*************************************************************************/

/*----------------------- Basic block processing ------------------------*/

/**** Basic block processing function declarations ****/

/**
 * rtl_block_add:  Add a new, empty basic block to the given unit at the
 * end of the unit->blocks[] array.
 *
 * [Parameters]
 *     unit: RTL unit.
 * [Return value]
 *     True on success, false on error.
 */
extern bool rtl_block_add(RTLUnit *unit);

/**
 * rtl_block_add_edge:  Add a new edge between two basic blocks.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     from_index: Index of dominating basic block (in unit->blocks[]).
 *     to_index: Index of dominated basic block (in unit->blocks[]).
 * [Return value]
 *     True on success, false on error.
 */
extern bool rtl_block_add_edge(RTLUnit *unit, int from_index, int to_index);

/**
 * rtl_block_remove_edge:  Remove an edge between two basic blocks.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     from_index: Index of dominating basic block (in unit->blocks[]).
 *     exit_index: Index of edge to remove in the dominating basic block's
 *         exit list (blocks[from_index].exits[]).
 */
extern void rtl_block_remove_edge(RTLUnit *unit, int from_index,
                                  int exit_index);

/*------------------------ Instruction encoding -------------------------*/

/* Internal table used by rtl_insn_make(). */
extern bool (* const makefunc_table[])(RTLUnit *, RTLInsn *, unsigned int,
                                       uint32_t, uint32_t, uint64_t);

/**
 * rtl_insn_make:  Fill in an RTLInsn structure based on the opcode stored
 * in the structure and the parameters passed to the function.
 *
 * [Parameters]
 *     unit: RTLUnit containing instruction.
 *     insn: RTLInsn structure to fill in (insn->opcode must be set by caller).
 *     dest: Destination register for instruction.
 *     src1: First source register for instruction.
 *     src2: Second source register for instruction.
 *     other: Extra register or immediate value for instruction.
 * [Return value]
 *     True on success, false on error.
 */
static inline bool rtl_insn_make(RTLUnit *unit, RTLInsn *insn,
                                 unsigned int dest, uint32_t src1,
                                 uint32_t src2, uint64_t other)
{
    ASSERT(unit != NULL);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);
    ASSERT(unit->label_blockmap != NULL);
    ASSERT(insn->opcode >= RTLOP__FIRST && insn->opcode <= RTLOP__LAST);
    ASSERT(makefunc_table[insn->opcode]);

    return (*makefunc_table[insn->opcode])(unit, insn,
                                           dest, src1, src2, other);
}

/*---------------------------- Optimization -----------------------------*/

/**
 * rtl_opt_fold_constants:  Perform constant folding on the given RTL unit,
 * converting instructions that operate on constant operands into load-
 * immediate instructions that load the result of the operation.  If such
 * an operand is not used by any other instruction, the instruction that
 * loaded it is changed to a NOP.
 *
 * [Parameters]
 *     unit: RTL unit
 * [Return value]
 *     Nonzero on success, zero on error
 */
extern int rtl_opt_fold_constants(RTLUnit *unit);

/**
 * rtl_opt_decondition:  Perform "deconditioning" of conditional branches
 * with constant conditions.  For "GOTO_IF_Z (GOTO_IF_NZ) label, rN" where
 * rN is type RTLREG_CONSTANT, the instruction is changed to GOTO if the
 * value of rN is zero (nonzero) and changed to NOP otherwise.  As with
 * constant folding, if the condition register is not used anywhere else,
 * the register is eliminated and  the instruction that loaded it is
 * changed to a NOP.
 *
 * [Parameters]
 *     unit: RTL unit
 * [Return value]
 *     Nonzero on success, zero on error
 */
extern int rtl_opt_decondition(RTLUnit *unit);

/**
 * rtl_opt_drop_dead_blocks:  Search an RTL unit for basic blocks which are
 * unreachable via any path from the initial block and remove them from the
 * code stream.  All blocks dominated only by such dead blocks are
 * recursively removed as well.
 *
 * [Parameters]
 *     unit: RTL unit
 * [Return value]
 *     Nonzero on success, zero on error
 */
extern int rtl_opt_drop_dead_blocks(RTLUnit *unit);

/**
 * rtl_opt_drop_dead_branches:  Search an RTL unit for branch instructions
 * which branch to the next instruction in the code stream and replace them
 * with NOPs.
 *
 * [Parameters]
 *     unit: RTL unit
 * [Return value]
 *     Nonzero on success, zero on error
 */
extern int rtl_opt_drop_dead_branches(RTLUnit *unit);

/*------------------------ Register information -------------------------*/

/**
 * rtl_register_is_int:  Return whether the given register has an integral
 * type.
 */
static inline bool rtl_register_is_int(const RTLRegister *reg)
{
    const uint32_t int_types =
        1U << RTLTYPE_INT32 |
        1U << RTLTYPE_ADDRESS;
    return (int_types & (1U << reg->type)) != 0;
}

/**
 * rtl_register_is_float:  Return whether the given register has a
 * floating-point type.
 */
static inline bool rtl_register_is_float(const RTLRegister *reg)
{
    const uint32_t float_types =
        1U << RTLTYPE_FLOAT |
        1U << RTLTYPE_DOUBLE |
        1U << RTLTYPE_V2_DOUBLE;
    return (float_types & (1U << reg->type)) != 0;
}

/*************************************************************************/
/*************************************************************************/

#endif  // SRC_RTL_INTERNAL_H

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
