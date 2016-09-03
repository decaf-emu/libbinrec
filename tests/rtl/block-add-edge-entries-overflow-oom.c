/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT_EQ(unit->num_blocks, 0);

    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add(unit));
    EXPECT_EQ(unit->num_blocks, 9);

    EXPECT(rtl_block_add_edge(unit, 0, 0));
    EXPECT(rtl_block_add_edge(unit, 1, 0));
    EXPECT(rtl_block_add_edge(unit, 2, 0));
    EXPECT(rtl_block_add_edge(unit, 3, 0));
    EXPECT(rtl_block_add_edge(unit, 4, 0));
    EXPECT(rtl_block_add_edge(unit, 5, 0));
    EXPECT(rtl_block_add_edge(unit, 6, 0));
    /* Include a case where the exit edge is not in the first slot. */
    EXPECT(rtl_block_add_edge(unit, 7, 7));
    EXPECT(rtl_block_add_edge(unit, 7, 0));
    EXPECT_EQ(unit->blocks[0].entries[0], 0);
    EXPECT_EQ(unit->blocks[0].entries[1], 1);
    EXPECT_EQ(unit->blocks[0].entries[2], 2);
    EXPECT_EQ(unit->blocks[0].entries[3], 3);
    EXPECT_EQ(unit->blocks[0].entries[4], 4);
    EXPECT_EQ(unit->blocks[0].entries[5], 5);
    EXPECT_EQ(unit->blocks[0].entries[6], 6);
    EXPECT_EQ(unit->blocks[0].entries[7], 7);
    EXPECT_EQ(unit->blocks[0].exits[0], 0);
    EXPECT_EQ(unit->blocks[0].exits[1], -1);
    EXPECT_EQ(unit->blocks[1].entries[0], -1);
    EXPECT_EQ(unit->blocks[1].exits[0], 0);
    EXPECT_EQ(unit->blocks[1].exits[1], -1);
    EXPECT_EQ(unit->blocks[2].entries[0], -1);
    EXPECT_EQ(unit->blocks[2].exits[0], 0);
    EXPECT_EQ(unit->blocks[2].exits[1], -1);
    EXPECT_EQ(unit->blocks[3].entries[0], -1);
    EXPECT_EQ(unit->blocks[3].exits[0], 0);
    EXPECT_EQ(unit->blocks[3].exits[1], -1);
    EXPECT_EQ(unit->blocks[4].entries[0], -1);
    EXPECT_EQ(unit->blocks[4].exits[0], 0);
    EXPECT_EQ(unit->blocks[4].exits[1], -1);
    EXPECT_EQ(unit->blocks[5].entries[0], -1);
    EXPECT_EQ(unit->blocks[5].exits[0], 0);
    EXPECT_EQ(unit->blocks[5].exits[1], -1);
    EXPECT_EQ(unit->blocks[6].entries[0], -1);
    EXPECT_EQ(unit->blocks[6].exits[0], 0);
    EXPECT_EQ(unit->blocks[6].exits[1], -1);
    EXPECT_EQ(unit->blocks[7].entries[0], 7);
    EXPECT_EQ(unit->blocks[7].entries[1], -1);
    EXPECT_EQ(unit->blocks[7].exits[0], 7);
    EXPECT_EQ(unit->blocks[7].exits[1], 0);
    EXPECT_EQ(unit->blocks[8].entries[0], -1);
    EXPECT_EQ(unit->blocks[8].exits[0], -1);
    EXPECT_STREQ(get_log_messages(), NULL);

    unit->blocks_size = 9;
    mem_wrap_fail_after(0);
    EXPECT_FALSE(rtl_block_add_edge(unit, 8, 0));
    EXPECT_ICE("Failed to add dummy unit");
    EXPECT_EQ(unit->num_blocks, 9);
    EXPECT_EQ(unit->blocks[0].entries[0], 0);
    EXPECT_EQ(unit->blocks[0].entries[1], 1);
    EXPECT_EQ(unit->blocks[0].entries[2], 2);
    EXPECT_EQ(unit->blocks[0].entries[3], 3);
    EXPECT_EQ(unit->blocks[0].entries[4], 4);
    EXPECT_EQ(unit->blocks[0].entries[5], 5);
    EXPECT_EQ(unit->blocks[0].entries[6], 6);
    EXPECT_EQ(unit->blocks[0].entries[7], 7);
    EXPECT_EQ(unit->blocks[0].exits[0], 0);
    EXPECT_EQ(unit->blocks[0].exits[1], -1);
    EXPECT_EQ(unit->blocks[1].entries[0], -1);
    EXPECT_EQ(unit->blocks[1].exits[0], 0);
    EXPECT_EQ(unit->blocks[1].exits[1], -1);
    EXPECT_EQ(unit->blocks[2].entries[0], -1);
    EXPECT_EQ(unit->blocks[2].exits[0], 0);
    EXPECT_EQ(unit->blocks[2].exits[1], -1);
    EXPECT_EQ(unit->blocks[3].entries[0], -1);
    EXPECT_EQ(unit->blocks[3].exits[0], 0);
    EXPECT_EQ(unit->blocks[3].exits[1], -1);
    EXPECT_EQ(unit->blocks[4].entries[0], -1);
    EXPECT_EQ(unit->blocks[4].exits[0], 0);
    EXPECT_EQ(unit->blocks[4].exits[1], -1);
    EXPECT_EQ(unit->blocks[5].entries[0], -1);
    EXPECT_EQ(unit->blocks[5].exits[0], 0);
    EXPECT_EQ(unit->blocks[5].exits[1], -1);
    EXPECT_EQ(unit->blocks[6].entries[0], -1);
    EXPECT_EQ(unit->blocks[6].exits[0], 0);
    EXPECT_EQ(unit->blocks[6].exits[1], -1);
    EXPECT_EQ(unit->blocks[7].entries[0], 7);
    EXPECT_EQ(unit->blocks[7].entries[1], -1);
    EXPECT_EQ(unit->blocks[7].exits[0], 7);
    EXPECT_EQ(unit->blocks[7].exits[1], 0);
    EXPECT_EQ(unit->blocks[8].entries[0], -1);
    EXPECT_EQ(unit->blocks[8].exits[0], -1);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
