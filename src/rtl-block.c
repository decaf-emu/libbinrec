/*
 * libbinjit: a just-in-time recompiler for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

/*
 * This source file contains the logic used to divide a stream of RTL
 * instructions into basic blocks.  Each basic block has exactly one entry
 * point and one exit point, though there may be multiple paths into or
 * out of the block.  As such, the block can be optimized and translated
 * to the target architecture without needing to track every possible path
 * to each instruction.
 */

#include "src/common.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"

#include <stdio.h>

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

/**
 * get_new_block:  Return the index of the next unused slot in unit->blocks[],
 * extending the block array if necessary.
 *
 * [Parameters]
 *     unit: RTLUnit to operate on.
 * [Return value]
 *     Block array index, or -1 if out of memory.
 */
static int get_new_block(RTLUnit *unit)
{
    ASSERT(unit != NULL);

    if (UNLIKELY(unit->num_blocks >= unit->blocks_size)) {
        int new_blocks_size = unit->num_blocks + BLOCKS_EXPAND_SIZE;
        RTLBlock *new_blocks = rtl_realloc(
            unit, unit->blocks, sizeof(*unit->blocks) * new_blocks_size);
        if (UNLIKELY(!new_blocks)) {
            log_error(unit->handle, "No memory to expand unit to %d blocks",
                      new_blocks_size);
            return -1;
        }
        unit->blocks = new_blocks;
        unit->blocks_size = new_blocks_size;
    }

    return unit->num_blocks++;
}

/*************************************************************************/
/********************** Internal interface routines **********************/
/*************************************************************************/

bool rtl_block_add(RTLUnit *unit)
{
    ASSERT(unit != NULL);

    const int index = get_new_block(unit);
    if (UNLIKELY(index < 0)) {
        return false;
    }

    unit->blocks[index].next_block = -1;
    unit->blocks[index].prev_block = unit->last_block;
    if (unit->last_block >= 0) {
        unit->blocks[unit->last_block].next_block = index;
    }
    unit->last_block = index;

    unit->blocks[index].first_insn = 0;
    unit->blocks[index].last_insn = -1;
    for (int i = 0; i < lenof(unit->blocks[index].entries); i++) {
        unit->blocks[index].entries[i] = -1;
    }
    unit->blocks[index].entry_overflow = -1;
    for (int i = 0; i < lenof(unit->blocks[index].exits); i++) {
        unit->blocks[index].exits[i] = -1;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

bool rtl_block_add_edge(RTLUnit *unit, int from_index, int to_index)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(from_index >= 0 && from_index < unit->num_blocks);
    ASSERT(to_index >= 0 && to_index < unit->num_blocks);

    int i;

    /* Find an empty exit slot; also check whether this edge already exists,
     * and return successfully (without adding a duplicate edge) if so.
     * A block can never have more than two exits, so bail if we try to add
     * a third. */
    for (i = 0; i < lenof(unit->blocks[from_index].exits); i++) {
        if (unit->blocks[from_index].exits[i] < 0) {
            break;
        } else if (unit->blocks[from_index].exits[i] == to_index) {
            return true;
        }
    }
    if (UNLIKELY(i >= lenof(unit->blocks[from_index].exits))) {
        log_ice(unit->handle, "Too many exits from block %u", from_index);
        return false;
    }
    const int exit_index = i;
    unit->blocks[from_index].exits[exit_index] = to_index;

    /* If we overflow the entry list, add a dummy block to take some of the
     * entries out. */
    for (i = 0; i < lenof(unit->blocks[to_index].entries); i++) {
        if (unit->blocks[to_index].entries[i] < 0) {
            break;
        }
    }
    if (LIKELY(i < lenof(unit->blocks[to_index].entries))) {
        unit->blocks[to_index].entries[i] = from_index;
    } else {
        /* No room in the entry list, so we need to create a dummy block. */
        const int dummy_block = get_new_block(unit);
        if (UNLIKELY(dummy_block < 0)) {
            log_ice(unit->handle, "Failed to add dummy unit");
            /* Careful not to leave a dangling edge! */
            unit->blocks[from_index].exits[exit_index] = -1;
            return false;
        }
        unit->blocks[dummy_block].first_insn = 0;
        unit->blocks[dummy_block].last_insn = -1;
        /* Insert at the beginning of the target's extension block list,
         * just for convenience (order isn't important). */
        unit->blocks[dummy_block].entry_overflow =
            unit->blocks[to_index].entry_overflow;
        unit->blocks[to_index].entry_overflow = dummy_block;
        /* The next/prev fields and exit array should never be referenced;
         * stick unique values in them to help debugging just in case. */
        unit->blocks[dummy_block].next_block = -2;
        unit->blocks[dummy_block].prev_block = -3;
        unit->blocks[dummy_block].exits[0] = -4;
        unit->blocks[dummy_block].exits[1] = -5;
        /* Move all the current edges over to the dummy block, except for
         * the first one (to preserve the invariant that the first entry
         * edge is the fallthrough edge). */
        for (i = 1; i < lenof(unit->blocks[to_index].entries); i++) {
            unit->blocks[dummy_block].entries[i] =
                unit->blocks[to_index].entries[i];
            unit->blocks[to_index].entries[i] = -1;
        }
        /* Fill in the remaining slot of the dummy block's entry array with
         * the new edge we're adding. */
        unit->blocks[dummy_block].entries[0] = from_index;
    }

    return true;
}

/*-----------------------------------------------------------------------*/

void rtl_block_remove_edge(RTLUnit *unit, int from_index, int exit_index)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(from_index >= 0);
    ASSERT(from_index < unit->num_blocks);
    ASSERT(exit_index >= 0);
    ASSERT(exit_index < lenof(unit->blocks[from_index].exits));
    ASSERT(unit->blocks[from_index].exits[exit_index] >= 0);

    RTLBlock * const from_block = &unit->blocks[from_index];
    const int to_index = from_block->exits[exit_index];
    RTLBlock *to_block = &unit->blocks[to_index];

    for (; exit_index < lenof(from_block->exits) - 1; exit_index++) {
        from_block->exits[exit_index] = from_block->exits[exit_index + 1];
    }
    from_block->exits[lenof(from_block->exits) - 1] = -1;

    int entry_index = 0;
    while (to_block->entries[entry_index] != from_index) {
        entry_index++;
        if (entry_index >= lenof(to_block->entries)) {
            ASSERT(to_block->entry_overflow >= 0);
            to_block = &unit->blocks[to_block->entry_overflow];
            entry_index = 0;
        }
    }

    for (; entry_index < lenof(to_block->entries) - 1; entry_index++) {
        to_block->entries[entry_index] = to_block->entries[entry_index + 1];
    }
    to_block->entries[lenof(to_block->entries) - 1] = -1;
}

/*************************************************************************/
/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
