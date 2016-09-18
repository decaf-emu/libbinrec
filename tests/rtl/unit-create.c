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

    EXPECT_PTREQ(unit->handle, handle);
    EXPECT(unit->blocks);
    EXPECT(unit->blocks_size > 0);
    EXPECT_EQ(unit->num_blocks, 0);
    EXPECT_FALSE(unit->have_block);
    EXPECT(unit->label_blockmap);
    EXPECT_EQ(unit->next_label, 1);
    EXPECT(unit->regs);
    EXPECT_EQ(unit->next_reg, 1);
    EXPECT(unit->aliases);
    EXPECT_EQ(unit->next_alias, 1);
    EXPECT_FALSE(unit->finalized);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
