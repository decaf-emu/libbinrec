/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    EXPECT(rtl_finalize_unit(unit));

    mem_wrap_fail_after(0);
    EXPECT_FALSE(rtl_optimize_unit(unit, 0));
    mem_wrap_cancel_fail();

    EXPECT_STREQ(get_log_messages(),
                 "[error] Failed to allocate blocks_seen (3 bytes)\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
