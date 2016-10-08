/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/common.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    EXPECT_EQ(handle->max_inline_length, 0);
    EXPECT_EQ(handle->max_inline_depth, 1);

    binrec_set_max_inline_length(handle, 10);
    EXPECT_EQ(handle->max_inline_length, 10);
    EXPECT_EQ(handle->max_inline_depth, 1);
    EXPECT_STREQ(get_log_messages(), NULL);

    binrec_set_max_inline_depth(handle, 2);
    EXPECT_EQ(handle->max_inline_length, 10);
    EXPECT_EQ(handle->max_inline_depth, 2);
    EXPECT_STREQ(get_log_messages(), NULL);

    binrec_set_max_inline_length(handle, -1);  // Invalid.
    EXPECT_EQ(handle->max_inline_length, 10);
    EXPECT_STREQ(get_log_messages(),
                 "[error] Invalid maximum inline length -1\n");
    clear_log_messages();

    binrec_set_max_inline_depth(handle, -1);  // Invalid.
    EXPECT_EQ(handle->max_inline_depth, 2);
    EXPECT_STREQ(get_log_messages(),
                 "[error] Invalid maximum inline depth -1\n");
    clear_log_messages();

    binrec_set_max_inline_length(handle, 0);
    binrec_set_max_inline_depth(handle, 0);
    EXPECT_EQ(handle->max_inline_length, 0);
    EXPECT_EQ(handle->max_inline_depth, 0);
    EXPECT_STREQ(get_log_messages(), NULL);

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
