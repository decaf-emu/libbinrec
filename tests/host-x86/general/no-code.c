/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/common.h"
#include "tests/host-x86/common.h"
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    EXPECT_FALSE(host_x86_translate(handle, unit));
    EXPECT_STREQ(get_log_messages(), "[error] No code to translate\n");
    clear_log_messages();

    EXPECT(rtl_block_add(unit));
    EXPECT_FALSE(host_x86_translate(handle, unit));
    EXPECT_STREQ(get_log_messages(), "[error] No code to translate\n");
    clear_log_messages();

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
