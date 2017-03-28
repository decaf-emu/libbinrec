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


static void *log_userdata;
static int log_level;
static char log_buf[1001];
static int last_level;
static char last_buf[1001];

static void log_callback(void *userdata, binrec_loglevel_t level,
                         const char *message)
{
    last_level = log_level;
    strcpy(last_buf, log_buf);

    log_userdata = userdata;
    log_level = level;
    ASSERT(strlen(message) < sizeof(log_buf));
    strcpy(log_buf, message);
}


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_callback;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT(handle->setup.log == log_callback);

    log_userdata = NULL;
    log_level = -1;
    *log_buf = '\0';
    handle->setup.userdata = (void *)123;
    log_info(handle, "test %d", 321);
    EXPECT_EQ((uintptr_t)log_userdata, 123);
    EXPECT_EQ(log_level, BINREC_LOGLEVEL_INFO);
    EXPECT_STREQ(log_buf, "test 321");

    log_userdata = NULL;
    log_level = -1;
    *log_buf = '\0';
    handle->setup.userdata = (void *)456;
    log_warning(handle, "test %d", 654);
    EXPECT_EQ((uintptr_t)log_userdata, 456);
    EXPECT_EQ(log_level, BINREC_LOGLEVEL_WARNING);
    EXPECT_STREQ(log_buf, "test 654");

    log_userdata = NULL;
    log_level = -1;
    *log_buf = '\0';
    handle->setup.userdata = (void *)789;
    log_error(handle, "test %d", 987);
    EXPECT_EQ((uintptr_t)log_userdata, 789);
    EXPECT_EQ(log_level, BINREC_LOGLEVEL_ERROR);
    EXPECT_STREQ(log_buf, "test 987");

    *log_buf = '\0';
    log_info(handle, "%-1000s", "test");
    EXPECT_EQ(last_level, BINREC_LOGLEVEL_WARNING);
    EXPECT_STREQ(last_buf, "Next log message is truncated");
    EXPECT_EQ(log_level, BINREC_LOGLEVEL_INFO);
    EXPECT_EQ(strlen(log_buf), 999);
    EXPECT_EQ(log_buf[0], 't');
    EXPECT_EQ(log_buf[1], 'e');
    EXPECT_EQ(log_buf[2], 's');
    EXPECT_EQ(log_buf[3], 't');
    for (int i = 4; i <= 998; i++) {
        if (log_buf[i] != ' ') {
            FAIL("log_buf[%d] was %d but should have been %d",
                 i, log_buf[i], ' ');
        }
    }
    EXPECT_EQ(log_buf[999], '\0');

    binrec_destroy_handle(handle);

    /* Make sure attempting to log with a null pointer doesn't crash. */
    setup.log = NULL;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT(handle->setup.log == NULL);
    log_info(handle, "test");
    binrec_destroy_handle(handle);

    return EXIT_SUCCESS;
}
