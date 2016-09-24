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
    setup.guest = BINREC_ARCH_PPC_7XX;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    binrec_set_code_range(handle, 0x1000, 0x1FFF);

    EXPECT_FALSE(binrec_translate(handle, 0xFFF, (void *[1]){}, (long[1]){}));
    EXPECT_STREQ(get_log_messages(), "[error] Address 0xFFF not within code"
                 " range 0x1000-0x1FFF\n");

    clear_log_messages();
    EXPECT_FALSE(binrec_translate(handle, 0x2000, (void *[1]){}, (long[1]){}));
    EXPECT_STREQ(get_log_messages(), "[error] Address 0x2000 not within code"
                 " range 0x1000-0x1FFF\n");

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
