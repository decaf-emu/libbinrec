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

#ifndef SRC_RTL_H
    #include "src/rtl.h"
#endif

/*************************************************************************/
/************************* Configuration options *************************/
/*************************************************************************/

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

/**
 * RTL_DEBUG_OPTIMIZE:  If defined, optimization functions will output
 * debugging information via log_info().
 */
#define RTL_DEBUG_OPTIMIZE

/*************************************************************************/
/*************************** Type declarations ***************************/
/*************************************************************************/

/**
 * RTLInsn:  A single platform-neutral (more or less) operation.  Guest
 * instructions are translated into sequences of RTLInsns, which are then
 * optimized and retranslated into host instructions.
 */
typedef struct RTLInsn {
    uint16_t opcode;          // Operation code (RTLOpcode)
    uint16_t dest;            // Destination register
    uint16_t src1, src2;      // Source registers
    union {
        struct {
            union {
                uint16_t src3;      // Third source register
                struct {
                    uint8_t start;  // First (lowest) bit number for a bitfield
                    uint8_t count;  // Number of bits for a bitfield
                } bitfield;
                int16_t offset;     // Byte offset for load/store instructions
                uint16_t alias;     // Alias index for SET/GET_ALIAS
                uint8_t arg_index;  // Argument index for LOAD_ARG
                uint16_t label;     // GOTO target label
            };
            uint16_t host_data_16;  // For use by host translators
            uint32_t host_data_32;  // For use by host translators
        };
        uint64_t src_imm;     // Source immediate value
    };
} RTLInsn;

STATIC_ASSERT(sizeof(RTLInsn) == 16, "RTLInsn should be 16 bytes long for best performance");

/*----------------------------------*/

/**
 * RTLRegType:  The type (source information) of a register used in an RTL
 * unit.
 */
typedef enum RTLRegType_ {
    RTLREG_UNDEFINED = 0,       // Not yet defined to anything
    RTLREG_CONSTANT,            // Constant value
    RTLREG_CONSTANT_NOFOLD,     // Constant value (folding disallowed)
    RTLREG_FUNC_ARG,            // Function argument
    RTLREG_MEMORY,              // Memory reference
    RTLREG_ALIAS,               // Loaded from an alias register
    RTLREG_RESULT,              // Result of an operation on other registers
    RTLREG_RESULT_NOFOLD,       // Result of operation (not constant foldable)
} RTLRegType;

/**
 * RTLRegister:  Data about a virtual register used in an RTL unit.
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

    /* True if this register should never be spilled. */
    bool unspillable;

    /* Liveness information. */
    bool live;                  // True if this register has been referenced
                                //    (never cleared once set, except if the
                                //    register is killed during optimization)
    uint16_t live_link;         // Next register in live list (sorted by birth)
    int32_t birth;              // First RTL insn index when register is live
                                //    (= insn where it's assigned, because SSA)
    int32_t death;              // Last RTL insn index when register is live

    /* Register value information. */
    union {
        union {                 // Value of register for RTLREG_CONSTANT
            uint64_t i64;
            float f32;
            double f64;
        } value;
        uint8_t arg_index;      // Function argument index for RTLREG_FUNC_ARG
        struct {
            uint16_t base;      // Register holding address for RTLREG_MEMORY
            int16_t offset;     // Access offset
            bool byterev;       // True if a byte-reversed access
            uint8_t size;       // Access size in bytes (1 or 2) if this is a
                                //    narrow integer load, else 0
            bool is_signed;     // True if a signed narrow integer load
        } memory;
        struct {
            uint16_t src;       // Source alias register
        } alias;
        struct {
            uint16_t
                opcode : 15,    // Operation code for RTLREG_RESULT{,_NOFOLD}
                is_imm : 1;     // True if the second operand is src_imm
            uint16_t src1;      // Operand 1
            union {
                int32_t src_imm;    // Immediate operand
                struct {
                    uint16_t src2;  // Operand 2
                    union {
                        uint16_t src3;  // Operand 3
                        struct {
                            uint8_t start;  // Start bit for bitfields
                            uint8_t count;  // Bit count for bitfields
                        };
                    };
                };
            };
        } result;
    };
};

/*----------------------------------*/

/**
 * RTLAlias:  Data about an alias register
 */
typedef struct RTLAlias_ {
    RTLDataType type;   // Data type (RTLTYPE_*)
    uint16_t base;      // Base register for loads and stores, or zero to
                        //    allocate transient storage for the alias
    int16_t offset;     // Access offset for loads/stores (ignored if base==0)
} RTLAlias;

/*----------------------------------*/

/**
 * RTLBlock:  Information about an basic block of code (a sequence of
 * instructions with one entry point and one exit point).  Note that a block
 * can be empty, denoted by last_insn < first_insn, and that last_insn can
 * be negative, if first_insn is 0 and the block is empty.
 */
typedef struct RTLBlock {
    /* unit->insns[] index of the first instruction in the block. */
    int32_t first_insn;
    /* unit->insns[] index of the last instruction in the block.
     * last_insn < first_insn indicates an empty block. */
    int32_t last_insn;

    /* If there are any entering control flow edges from blocks located
     * after this block in the code stream, this field gives the
     * unit->insns[] index of the last instruction in the latest of those
     * blocks, which is the minimum instruction through which any register
     * live on entry to this block must be live.  Otherwise, this field is
     * set to zero. */
    int32_t min_death;

    /* unit->blocks[] index of the next block in the code stream (excluding
     * dropped blocks); -1 indicates the end of the code stream. */
    int16_t next_block;
    /* unit->blocks[] index of the previous block in the code stream, or
     * -1 if this is the first block. */
    int16_t prev_block;

    /* unit->blocks[] indices of predecessor blocks in the control flow
     * graph; -1 indicates an unused slot.  Holes in the list are not
     * permitted.  entries[0] is always the fall-through edge from the
     * previous block, if that edge exists.  (rtl_block_*() functions take
     * care of all these details.) */
    int16_t entries[7];

    /* unit->blocks[] index of an extension block used to hold additional
     * entry edges that don't fit in entries[], or -1 if none.  See
     * RTLExtraEntryBlock for format. */
    int16_t entry_overflow;

    /* unit->blocks[] indices of successor blocks.  Note that a terminating
     * insstruction can go at most two places (conditional GOTO: branch
     * target and fall-through path).  exits[0] is always the fall-through
     * edge if that edge exists. */
    int16_t exits[2];
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
    bool have_block;            // True if there is a currently active block
    uint16_t cur_block;         // Current block index if have_block != 0
    int16_t last_block;         // Most recently added block (excluding dummy
                                //    blocks used for entry list extension),
                                //    or -1 if none

    int16_t *label_blockmap;    // Label-to-block-index mapping (-1 = unset)
    uint16_t labels_size;       // Size of label-to-block map array (entries)
    uint16_t next_label;        // Next label number to allocate
                                //    (== number of allocated labels)

    RTLRegister *regs;          // Register array
    uint16_t regs_size;         // Size of register array (entries)
    uint16_t next_reg;          // Next register number to allocate
                                //    (== number of allocated registers)
    uint16_t first_live_reg;    // First register in live range list
    uint16_t last_live_reg;     // Last register in live range list

    RTLAlias *aliases;          // Alias register array
    uint16_t aliases_size;      // Size of alias register array (entries)
    uint16_t next_alias;        // Next alias register number to allocate
                                //    (== number of allocated alias registers)

    bool error;                 // True if an error has been detected
    bool finalized;             // True if unit has been finalized

    char *disassembly;          // Last disassembly result, or NULL if none
                                //    (saved so we can free it at destroy time)

    /* The following fields are used only by optimization routines: */
    bool *block_seen;           // Array of "seen" flags for all blocks
                                //    (used by rtlopt_drop_dead_blocks())
    int block_seen_size;        // Allocated size of block_seen array
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
#define rtl_block_add INTERNAL(rtl_block_add)
extern bool rtl_block_add(RTLUnit *unit);

/**
 * rtl_block_add_edge:  Add a new control flow edge between two basic blocks.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     from_index: Index of dominating basic block (in unit->blocks[]).
 *     to_index: Index of dominated basic block (in unit->blocks[]).
 * [Return value]
 *     True on success, false on error.
 */
#define rtl_block_add_edge INTERNAL(rtl_block_add_edge)
extern bool rtl_block_add_edge(RTLUnit *unit, int from_index, int to_index);

/**
 * rtl_block_remove_edge:  Remove a control flow edge between two basic blocks.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     from_index: Index of dominating basic block (in unit->blocks[]).
 *     exit_index: Index of edge to remove in the dominating basic block's
 *         exit list (blocks[from_index].exits[]).
 */
#define rtl_block_remove_edge INTERNAL(rtl_block_remove_edge)
extern void rtl_block_remove_edge(RTLUnit *unit, int from_index,
                                  int exit_index);

/*------------------------ Instruction encoding -------------------------*/

/* Internal table used by rtl_insn_make(). */
#define makefunc_table INTERNAL(makefunc_table)
extern bool (* const makefunc_table[])(RTLUnit *, RTLInsn *, int, int, int,
                                       uint64_t);

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
                                 int dest, int src1, int src2, uint64_t other)
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

/*-------------------------- Memory management --------------------------*/

/*
 * These functions wrap the similarly-named binrec_* memory management
 * functions declared in common.h; these allow us to pass an RTLUnit
 * directly without having to explicitly dereference it to get the
 * binrec handle.
 */

static inline void *rtl_malloc(const RTLUnit *unit, size_t size)
{
    ASSERT(unit);
    return binrec_malloc(unit->handle, size);
}

static inline void *rtl_realloc(const RTLUnit *unit, void *ptr, size_t size)
{
    ASSERT(unit);
    return binrec_realloc(unit->handle, ptr, size);
}

static inline void rtl_free(const RTLUnit *unit, void *ptr)
{
    ASSERT(unit);
    binrec_free(unit->handle, ptr);
}

/*---------------------------- Optimization -----------------------------*/

/**
 * rtl_opt_prev_reg_use:  Return the latest use of the given register prior
 * to the given instruction.  If the register is not referenced by any
 * instructions between its birth and the given instruction, the index of
 * its birth instruction is returned.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     reg_index: Index of register to check.
 *     insn_index: Instruction index to use as endpoint of check.
 * [Return value]
 *     Instruction index of the latest reference to reg_index before
 *     insn_index.
 */
extern PURE_FUNCTION int rtl_opt_prev_reg_use(
    const RTLUnit *unit, int reg_index, int insn_index);

/**
 * rtl_opt_kill_insn:  Replace the given instruction with a NOP.  If any
 * of the instruction's input operands died at that instruction, roll their
 * deaths back to the previous instruction at which they are referenced; if
 * such a register has no other references after the instruction that sets
 * it and if dse is true, recursively kill the instruction that sets it.
 *
 * Helper function for optimization routines.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     insn_index: Index of instruction to kill.
 *     dse: True to recursively kill stores which become dead; false to
 *         leave them alone.
 */
#define rtl_opt_kill_insn INTERNAL(rtl_opt_kill_insn)
extern void rtl_opt_kill_insn(RTLUnit *unit, int insn_index, bool dse);

/**
 * rtl_opt_alias_data_flow:  Analyze data flow through aliases in the given
 * unit and eliminate SET_ALIAS instructions which are not visible to any
 * subsequent alias references.  Aliases with bound storage (from
 * rtl_set_alias_storage()) are treated as being implicitly referenced
 * when the unit returns to its caller.
 *
 * This function's worst-case runtime is roughly quadratic in the number
 * of basic blocks in the unit (more precisely: O(ab^2+i), where a is the
 * number of aliases, b is the number of basic blocks, and i is the number
 * of instructions in the unit).
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_opt_alias_data_flow INTERNAL(rtl_opt_alias_data_flow)
extern void rtl_opt_alias_data_flow(RTLUnit *unit);

/**
 * rtl_opt_decondition:  Perform "deconditioning" of conditional branches
 * with constant conditions.  For "GOTO_IF_Z (GOTO_IF_NZ) label, rN" where
 * rN is type RTLREG_CONSTANT, the instruction is changed to GOTO if the
 * value of rN is zero (nonzero) and NOP otherwise.  As with constant
 * folding, if the condition register is not used anywhere else, the
 * register is eliminated and  the instruction that loaded it is changed
 * to a NOP.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_opt_decondition INTERNAL(rtl_opt_decondition)
extern void rtl_opt_decondition(RTLUnit *unit);

/**
 * rtl_opt_drop_dead_blocks:  Search an RTL unit for basic blocks which are
 * unreachable via any path from the initial block and remove them from the
 * code stream.  All blocks dominated only by such dead blocks are
 * recursively removed as well.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_opt_drop_dead_blocks INTERNAL(rtl_opt_drop_dead_blocks)
extern void rtl_opt_drop_dead_blocks(RTLUnit *unit);

/**
 * rtl_opt_drop_dead_branches:  Search an RTL unit for branch instructions
 * which branch to the next instruction in the code stream and replace them
 * with NOPs.
 *
 * [Parameters]
 *     unit: RTL unit.
 *     dse: True to eliminate dead stores resulting from dropped branches,
 *         false to leave dead stores alone.
 */
#define rtl_opt_drop_dead_branches INTERNAL(rtl_opt_drop_dead_branches)
extern void rtl_opt_drop_dead_branches(RTLUnit *unit, bool dse);

/**
 * rtl_opt_drop_dead_stores:  Search an RTL unit for instructions that
 * write to a register which is never used elsewhere, and replace them
 * with NOPs.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_opt_drop_dead_stores INTERNAL(rtl_opt_drop_dead_stores)
extern void rtl_opt_drop_dead_stores(RTLUnit *unit);

/**
 * rtl_opt_fold_constants:  Perform constant folding on the given RTL unit,
 * converting instructions that operate on constant operands into load-
 * immediate instructions that load the result of the operation.  If such
 * an operand is not used by any other instruction, the instruction that
 * loaded it is changed to a NOP.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_opt_fold_constants INTERNAL(rtl_opt_fold_constants)
extern void rtl_opt_fold_constants(RTLUnit *unit);

/**
 * rtl_opt_thread_branches:  Search an RTL unit for branch instructions
 * which directly target other (unconditional or same-conditioned) branch
 * instructions, and rewrite their targets to point to the final branch
 * target.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_opt_thread_branches INTERNAL(rtl_opt_thread_branches)
extern void rtl_opt_thread_branches(RTLUnit *unit);

/*------------------------ Register information -------------------------*/

/**
 * rtl_type_is_int:  Return whether the given data type is an integral type.
 */
static inline CONST_FUNCTION bool rtl_type_is_int(RTLDataType type)
{
    return type <= RTLTYPE_ADDRESS;
}

/**
 * rtl_type_is_scalar:  Return whether the given data type is a scalar type.
 */
static inline CONST_FUNCTION bool rtl_type_is_scalar(RTLDataType type)
{
    return type <= RTLTYPE_FLOAT64;
}

/**
 * rtl_register_is_int:  Return whether the given register has an integral
 * type.
 */
static inline PURE_FUNCTION bool rtl_register_is_int(const RTLRegister *reg)
{
    return rtl_type_is_int(reg->type);
}

/**
 * rtl_register_is_scalar:  Return whether the given register has a scalar
 * type.
 */
static inline PURE_FUNCTION bool rtl_register_is_scalar(const RTLRegister *reg)
{
    return rtl_type_is_scalar(reg->type);
}

/*------------------- Miscellaneous utility functions -------------------*/

/**
 * snprintf_assert:  Wrapper for snprintf() which ASSERT()s that the
 * written string fits within the supplied buffer.
 */
#define snprintf_assert INTERNAL(snprintf_assert)
FORMAT(3, 4)
extern int snprintf_assert(char *buf, size_t size, const char *format, ...);

/**
 * format_int:  Write the given integer value to the given buffer as a
 * numeric string in an appropriate format for the type and value magnitude.
 * Non-address types in the range [-0x8000,+0xFFFF] are formatted as decimal
 * values; all other cases are formatted in hexadecimal.
 *
 * [Parameters]
 *     buf: Buffer into which to store formatted value.
 *     bufsize: Size of buffer, in bytes.  Should be at least 19.
 *     type: Type of value (RTLTYPE_*).
 *     value: Value, as a 64-bit integer.
 * [Return value]
 *     Number of bytes written, like snprintf().
 */
#define format_int INTERNAL(format_int)
extern int format_int(char *buf, int bufsize, RTLDataType type, uint64_t value);

/**
 * format_float, format_double:  Write the given floating-point value to
 * the given buffer as a numeric string.
 *
 * [Parameters]
 *     buf: Buffer into which to store formatted value.
 *     bufsize: Size of buffer, in bytes.  Should be at least 14 for
 *         format_float(), 24 for format_double().
 *     value: Value to format.
 * [Return value]
 *     Number of bytes written, like snprintf().
 */
#define format_float INTERNAL(format_float)
extern int format_float(char *buf, int bufsize, float value);
#define format_double INTERNAL(format_double)
extern int format_double(char *buf, int bufsize, double value);

/**
 * rtl_type_name:  Return an appropriate name for the given data type.
 */
#define rtl_type_name INTERNAL(rtl_type_name)
extern CONST_FUNCTION const char *rtl_type_name(RTLDataType type);

/**
 * rtl_type_suffix:  Return an appropriate suffix for the given data type.
 */
#define rtl_type_suffix INTERNAL(rtl_type_suffix)
extern CONST_FUNCTION const char *rtl_type_suffix(RTLDataType type);

/**
 * rtl_source_name:  Return an appropriate name for the given register source.
 */
#define rtl_source_name INTERNAL(rtl_source_name)
extern CONST_FUNCTION const char *rtl_source_name(RTLRegType source);

/**
 * rtl_update_live_ranges:  Update the live range of any register live at
 * the beginning of a block targeted by a backward branch so that the
 * register is live through all branches that target the block.
 *
 * Worst-case execution time is O(n*m) in the number of blocks (n) and the
 * number of registers (m).  However, the register scan is only required
 * for blocks targeted by backward branches, and it terminates at the first
 * register born within or after the targeted block.
 *
 * [Parameters]
 *     unit: RTL unit.
 */
#define rtl_update_live_ranges INTERNAL(rtl_update_live_ranges)
extern void rtl_update_live_ranges(RTLUnit *unit);

/*************************************************************************/
/*************************************************************************/

#endif  // SRC_RTL_INTERNAL_H
