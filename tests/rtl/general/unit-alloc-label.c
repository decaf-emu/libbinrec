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

    EXPECT_EQ(rtl_alloc_label(unit), 1);
    EXPECT_EQ(unit->next_label, 2);
    EXPECT_EQ(unit->label_blockmap[1], -1);
    EXPECT_FALSE(unit->error);

    /* Check behavior when the label array needs to be expanded. */
    unit->labels_size = 2;
    EXPECT_EQ(rtl_alloc_label(unit), 2);
    EXPECT_EQ(unit->labels_size, 2 + LABELS_EXPAND_SIZE);
    EXPECT_EQ(unit->next_label, 3);
    EXPECT_EQ(unit->label_blockmap[1], -1);
    EXPECT_EQ(unit->label_blockmap[2], -1);
    EXPECT_FALSE(unit->error);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
