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
/********************** External interface routines **********************/
/*************************************************************************/

bool rtl_block_add(RTLUnit *unit)
{
    ASSERT(unit != NULL);

    if (UNLIKELY(unit->num_blocks >= unit->blocks_size)) {
        int new_blocks_size = unit->num_blocks + BLOCKS_EXPAND_SIZE;
        RTLBlock *new_blocks = realloc(unit->blocks,
                                     sizeof(*unit->blocks) * new_blocks_size);
        if (UNLIKELY(!new_blocks)) {
            log_error(unit->handle, "No memory to expand unit to %d blocks",
                      new_blocks_size);
            return false;
        }
        unit->blocks = new_blocks;
        unit->blocks_size = new_blocks_size;
    }

    const int index = unit->num_blocks++;
    if (index > 0) {
        unit->blocks[index-1].next_block = index;
    }
    unit->blocks[index].first_insn = 0;
    unit->blocks[index].last_insn = -1;
    unit->blocks[index].next_block = -1;
    unit->blocks[index].prev_block = index-1;
    unsigned int i;
    for (i = 0; i < lenof(unit->blocks[index].entries); i++) {
        unit->blocks[index].entries[i] = -1;
    }
    for (i = 0; i < lenof(unit->blocks[index].exits); i++) {
        unit->blocks[index].exits[i] = -1;
    }
    unit->blocks[index].next_call_block = -1;
    unit->blocks[index].prev_call_block = -1;

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
    }
    unit->blocks[from_index].exits[i] = to_index;

    /* If we overflow the entry list, add a dummy block to take some of the
     * entries out. */
    for (i = 0; i < lenof(unit->blocks[to_index].entries); i++) {
        if (unit->blocks[to_index].entries[i] < 0) {
            break;
        }
    }
    if (UNLIKELY(i >= lenof(unit->blocks[to_index].entries))) {
        /* No room in the entry list, so we need to create a dummy block. */
        if (UNLIKELY(!rtl_block_add(unit))) {
            log_ice(unit->handle, "Failed to add dummy unit");
            return false;
        }
        const int dummy_block = unit->num_blocks - 1;
        /* Move all the current edges over to the dummy block. */
        for (i = 0; i < lenof(unit->blocks[to_index].entries); i++) {
            const int other_block = unit->blocks[to_index].entries[i];
            int j;
            for (j = 0; j < lenof(unit->blocks[other_block].exits); j++) {
                if (unit->blocks[other_block].exits[j] == to_index) {
                    break;
                }
            }
            if (UNLIKELY(j >= lenof(unit->blocks[other_block].exits))) {
                log_ice(unit->handle, "Edge to unit %u missing from unit %u",
                        to_index, other_block);
                return false;
            }
            unit->blocks[other_block].exits[j] = dummy_block;
            unit->blocks[dummy_block].entries[i] = other_block;
        }
        /* Link to the original block. */
        unit->blocks[dummy_block].exits[0] = to_index;
        unit->blocks[dummy_block].exits[1] = -1;
        unit->blocks[to_index].entries[0] = dummy_block;
        /* The new entry will go into the second slot; clear out all
         * other edges. */
        for (i = lenof(unit->blocks[to_index].entries) - 1; i > 1; i--) {
            unit->blocks[to_index].entries[i] = -1;
        }
    }

    unit->blocks[to_index].entries[i] = from_index;

    return true;
}

/*-----------------------------------------------------------------------*/

void rtl_block_remove_edge(RTLUnit *unit, int from_index, int exit_index)
{
    ASSERT(unit != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(from_index < unit->num_blocks);
    ASSERT(exit_index < lenof(unit->blocks[from_index].exits));
    ASSERT(unit->blocks[from_index].exits[exit_index] >= 0);

    RTLBlock * const from_block = &unit->blocks[from_index];
    const int to_index = from_block->exits[exit_index];
    RTLBlock * const to_block = &unit->blocks[to_index];
    int entry_index;

    for (; exit_index < lenof(from_block->exits) - 1; exit_index++) {
        from_block->exits[exit_index] = from_block->exits[exit_index + 1];
    }
    from_block->exits[lenof(from_block->exits) - 1] = -1;

    for (entry_index = 0; entry_index < lenof(to_block->entries);
         entry_index++)
    {
        if (to_block->entries[entry_index] == from_index) {
            break;
        }
    }
    if (UNLIKELY(entry_index >= lenof(to_block->entries))) {
        log_ice(unit->handle, "Edge %u->%u missing from %u.entries",
                from_index, to_index, to_index);
        return;
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
