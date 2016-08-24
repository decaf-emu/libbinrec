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

    /* The code range should default to the entire address space. */
    EXPECT_EQ(handle->code_range_start, 0);
    EXPECT_EQ(handle->code_range_end, 0xFFFFFFFF);

    /* Check that we can set the range. */
    binrec_set_code_range(handle, 0x1000, 0x1FFF);
    EXPECT_EQ(handle->code_range_start, 0x1000);
    EXPECT_EQ(handle->code_range_end, 0x1FFF);

    /* Check that we can't set a 0-byte range, and that attempting to do so
     * signals an invalid state for binrec_translate(). */
    binrec_set_code_range(handle, 0x2000, 0x1FFF);
    EXPECT_EQ(handle->code_range_start, 1);
    EXPECT_EQ(handle->code_range_end, 0);

    /* Check that we can set a 1-byte range. */
    binrec_set_code_range(handle, 0x2000, 0x2000);
    EXPECT_EQ(handle->code_range_start, 0x2000);
    EXPECT_EQ(handle->code_range_end, 0x2000);

    /* Check that we can't set a wraparound range. */
    binrec_set_code_range(handle, -0x1000, 0x0FFF);
    EXPECT_EQ(handle->code_range_start, 1);
    EXPECT_EQ(handle->code_range_end, 0);

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
