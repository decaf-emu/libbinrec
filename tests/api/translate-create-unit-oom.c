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
#include "tests/mem-wrappers.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = BINREC_ARCH_PPC_7XX;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    binrec_set_code_range(handle, 0x1000, 0x1FFF);

    mem_wrap_fail_after(0);
    EXPECT_FALSE(binrec_translate(handle, 0x1000, (void *[1]){}, (long[1]){}));
    EXPECT_STREQ(get_log_messages(), ("[error] No memory for RTLUnit\n"
                                      "[error] Failed to create RTLUnit\n"));

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
