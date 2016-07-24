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


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    EXPECT_EQ(handle->max_inline_length, 0);
    EXPECT_EQ(handle->max_inline_depth, 1);

    binrec_set_max_inline_length(handle, 10);
    EXPECT_EQ(handle->max_inline_length, 10);
    EXPECT_EQ(handle->max_inline_depth, 1);

    binrec_set_max_inline_depth(handle, 2);
    EXPECT_EQ(handle->max_inline_length, 10);
    EXPECT_EQ(handle->max_inline_depth, 2);

    binrec_set_max_inline_length(handle, -1);  // Invalid.
    binrec_set_max_inline_depth(handle, 0);  // Invalid.
    EXPECT_EQ(handle->max_inline_length, 10);
    EXPECT_EQ(handle->max_inline_depth, 2);

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
