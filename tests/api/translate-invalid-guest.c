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
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.log = log_capture;
    binrec_t *handle;

    setup.guest = BINREC_ARCH_INVALID;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, NULL, 0, -1,
                                  (void *[1]){0}, (long[1]){0}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported guest architecture: (invalid"
                 " architecture)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    setup.guest = (binrec_arch_t)-1;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, NULL, 0, -1,
                                  (void *[1]){0}, (long[1]){0}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported guest architecture: (invalid"
                 " architecture)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    setup.guest = BINREC_ARCH_X86_64_SYSV;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, NULL, 0, -1,
                                  (void *[1]){0}, (long[1]){0}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported guest architecture: x86-64"
                 " (SysV ABI)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    setup.guest = BINREC_ARCH_X86_64_WINDOWS;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, NULL, 0, -1,
                                  (void *[1]){0}, (long[1]){0}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported guest architecture: x86-64"
                 " (Windows ABI)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    setup.guest = BINREC_ARCH_X86_64_WINDOWS_SEH;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, NULL, 0, -1,
                                  (void *[1]){0}, (long[1]){0}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported guest architecture: x86-64"
                 " (Windows ABI with unwind data)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    return EXIT_SUCCESS;
}
