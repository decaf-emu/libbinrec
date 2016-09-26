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


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT_EQ(unit->num_blocks, 0);
    EXPECT(unit->blocks_size > 0);
    const int blocks_size = unit->blocks_size;

    EXPECT(rtl_block_add(unit));
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT_EQ(unit->blocks_size, blocks_size);
    EXPECT_EQ(unit->blocks[0].first_insn, 0);
    EXPECT_EQ(unit->blocks[0].last_insn, -1);
    EXPECT_EQ(unit->blocks[0].entries[0], -1);
    EXPECT_EQ(unit->blocks[0].entries[1], -1);
    EXPECT_EQ(unit->blocks[0].entries[2], -1);
    EXPECT_EQ(unit->blocks[0].entries[3], -1);
    EXPECT_EQ(unit->blocks[0].entries[4], -1);
    EXPECT_EQ(unit->blocks[0].entries[5], -1);
    EXPECT_EQ(unit->blocks[0].entries[6], -1);
    EXPECT_EQ(unit->blocks[0].entries[7], -1);
    EXPECT_EQ(unit->blocks[0].exits[0], -1);
    EXPECT_EQ(unit->blocks[0].exits[1], -1);

    EXPECT_FALSE(unit->error);
    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
