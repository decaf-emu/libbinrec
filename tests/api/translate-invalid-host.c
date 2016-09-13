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
    setup.guest = BINREC_ARCH_POWERPC_750CL;
    setup.log = log_capture;
    binrec_t *handle;

    setup.host = BINREC_ARCH_INVALID;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, 0,
                                  (binrec_entry_t[1]){}, (long[1]){}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported host architecture: (invalid"
                 " architecture)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    setup.host = (binrec_arch_t)-1;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, 0,
                                  (binrec_entry_t[1]){}, (long[1]){}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported host architecture: (invalid"
                 " architecture)\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    setup.host = BINREC_ARCH_POWERPC_750CL;
    EXPECT(handle = binrec_create_handle(&setup));
    EXPECT_FALSE(binrec_translate(handle, 0,
                                  (binrec_entry_t[1]){}, (long[1]){}));
    EXPECT_STREQ(get_log_messages(),
                 "[error] Unsupported host architecture: PowerPC 750CL\n");
    binrec_destroy_handle(handle);
    clear_log_messages();

    return EXIT_SUCCESS;
}
