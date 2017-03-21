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

    /* Convenience constants. */
    const int map_len = lenof(handle->readonly_map);
    const int num_partial = lenof(handle->partial_readonly_pages);
    const int partial_len = lenof(handle->partial_readonly_map[0]);

    /* By default, no pages should be marked readonly. */
    for (int i = 0; i < map_len; i++) {
        if (handle->readonly_map[i] != 0) {
            FAIL("handle->readonly_map[%d] was not zero", i);
        }
    }
    EXPECT_EQ(handle->num_partial_readonly, 0);
    for (int i = 0; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }

    /* Check marking a full page. */
    EXPECT(binrec_add_readonly_region(handle, READONLY_PAGE_SIZE * 5,
                                      READONLY_PAGE_SIZE));
    EXPECT_EQ(handle->readonly_map[1], 2<<(2*1));
    for (int i = 0; i < map_len; i++) {
        if (i != 1 && handle->readonly_map[i] != 0) {
            FAIL("handle->readonly_map[%d] was not zero", i);
        }
    }
    EXPECT_EQ(handle->num_partial_readonly, 0);
    for (int i = 0; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check marking multiple pages. */
    EXPECT(binrec_add_readonly_region(handle, READONLY_PAGE_SIZE * 7,
                                      READONLY_PAGE_SIZE * 3));
    EXPECT_EQ(handle->readonly_map[1], 2<<(2*1) | 2<<(2*3));
    EXPECT_EQ(handle->readonly_map[2], 2<<(2*0) | 2<<(2*1));
    for (int i = 0; i < map_len; i++) {
        if (i != 1 && i != 2 && handle->readonly_map[i] != 0) {
            FAIL("handle->readonly_map[%d] was not zero", i);
        }
    }
    EXPECT_EQ(handle->num_partial_readonly, 0);
    for (int i = 0; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check marking a portion of a page. */
    EXPECT(binrec_add_readonly_region(handle, READONLY_PAGE_SIZE * 6 + 1,
                                      READONLY_PAGE_SIZE - 2));
    EXPECT_EQ(handle->readonly_map[1], 2<<(2*1) | 1<<(2*2) | 2<<(2*3));
    EXPECT_EQ(handle->readonly_map[2], 2<<(2*0) | 2<<(2*1));
    for (int i = 0; i < map_len; i++) {
        if (i != 1 && i != 2 && handle->readonly_map[i] != 0) {
            FAIL("readonly_map[%d] was not zero", i);
        }
    }
    EXPECT_EQ(handle->num_partial_readonly, 1);
    EXPECT_EQ(handle->partial_readonly_pages[0], 6);
    EXPECT_EQ(handle->partial_readonly_map[0][0], 0xFE);
    for (int i = 1; i < partial_len-1; i++) {
        if (handle->partial_readonly_map[0][i] != 0xFF) {
            FAIL("handle->partial_readonly_map[0][%d] was %d but should"
                 " have been 255", i, handle->partial_readonly_map[0][i]);
        }
    }
    EXPECT_EQ(handle->partial_readonly_map[0][partial_len-1], 0x7F);
    for (int i = 1; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check clearing marked regions. */
    binrec_clear_readonly_regions(handle);
    for (int i = 0; i < map_len; i++) {
        if (handle->readonly_map[i] != 0) {
            FAIL("handle->readonly_map[%d] was not zero", i);
        }
    }
    EXPECT_EQ(handle->num_partial_readonly, 0);
    for (int i = 0; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check marking from the end of one page to the beginning of another. */
    EXPECT(binrec_add_readonly_region(handle, READONLY_PAGE_SIZE * 2 - 1,
                                      READONLY_PAGE_SIZE + 2));
    EXPECT_EQ(handle->readonly_map[0], 1<<(2*1) | 2<<(2*2) | 1<<(2*3));
    for (int i = 1; i < map_len; i++) {
        if (handle->readonly_map[i] != 0) {
            FAIL("readonly_map[%d] was not zero", i);
        }
    }
    EXPECT_EQ(handle->num_partial_readonly, 2);
    EXPECT_EQ(handle->partial_readonly_pages[0], 1);
    for (int i = 0; i < partial_len-1; i++) {
        if (handle->partial_readonly_map[0][i] != 0) {
            FAIL("handle->partial_readonly_map[0][%d] was %d but should"
                 " have been 0", i, handle->partial_readonly_map[0][i]);
        }
    }
    EXPECT_EQ(handle->partial_readonly_map[0][partial_len-1], 0x80);
    EXPECT_EQ(handle->partial_readonly_pages[1], 3);
    EXPECT_EQ(handle->partial_readonly_map[1][0], 0x01);
    for (int i = 1; i < lenof(handle->partial_readonly_map[1]); i++) {
        if (handle->partial_readonly_map[1][i] != 0) {
            FAIL("handle->partial_readonly_map[1][%d] was %d but should"
                 " have been 0", i, handle->partial_readonly_map[1][i]);
        }
    }
    for (int i = 2; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check adding to an existing partial page. */
    EXPECT(binrec_add_readonly_region(handle, READONLY_PAGE_SIZE * 3 + 2, 1));
    EXPECT_EQ(handle->num_partial_readonly, 2);
    EXPECT_EQ(handle->partial_readonly_pages[1], 3);
    EXPECT_EQ(handle->partial_readonly_map[1][0], 0x05);
    for (int i = 1; i < lenof(handle->partial_readonly_map[1]); i++) {
        if (handle->partial_readonly_map[1][i] != 0) {
            FAIL("handle->partial_readonly_map[1][%d] was %d but should"
                 " have been 0", i, handle->partial_readonly_map[1][i]);
        }
    }
    for (int i = 2; i < num_partial; i++) {
        if (handle->partial_readonly_pages[i] != ~0U) {
            FAIL("handle->partial_readonly_pages[%d] was not unused", i);
        }
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    /* Check failure on full partial page table (all page addresses less
     * than target page). */
    for (int i = 0; i < num_partial; i++) {
        handle->partial_readonly_pages[i] = i;
    }
    handle->num_partial_readonly = num_partial;
    EXPECT_FALSE(binrec_add_readonly_region(
                     handle, num_partial * READONLY_PAGE_SIZE, 1));
    char expected_buf[1000];
    snprintf(expected_buf, sizeof(expected_buf),
             "[error] Failed to add read-only range [0x%X,0x%X]: no free"
             " partial page entries\n", num_partial * READONLY_PAGE_SIZE,
             num_partial * READONLY_PAGE_SIZE);
    EXPECT_STREQ(get_log_messages(), expected_buf);
    clear_log_messages();

    /* Check failure on full partial page table (some page addresses
     * greater than target page). */
    handle->partial_readonly_pages[num_partial - 1] = num_partial;
    EXPECT_FALSE(binrec_add_readonly_region(
                     handle, num_partial * READONLY_PAGE_SIZE - 1, 1));
    snprintf(expected_buf, sizeof(expected_buf),
             "[error] Failed to add read-only range [0x%X,0x%X]: no free"
             " partial page entries\n", num_partial * READONLY_PAGE_SIZE - 1,
             num_partial * READONLY_PAGE_SIZE - 1);
    EXPECT_STREQ(get_log_messages(), expected_buf);
    clear_log_messages();

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
