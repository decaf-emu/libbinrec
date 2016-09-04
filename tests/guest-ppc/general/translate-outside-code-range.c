/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    /* The range includes 3 bytes of the word at 0x4, but not the last byte. */
    binrec_set_code_range(handle, 0, 6);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    /* This should fail without even trying to read an instruction
     * (which would crash since we've left the base pointer at NULL). */
    EXPECT_FALSE(guest_ppc_translate(handle, 4, unit));

    EXPECT_STREQ(get_log_messages(), "[error] First instruction at 0x4 falls"
                 " outside code range\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
