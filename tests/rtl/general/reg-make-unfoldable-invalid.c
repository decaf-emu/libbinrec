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


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    EXPECT_EQ(rtl_alloc_register(unit, RTLTYPE_INT32), 1);

    EXPECT_EQ(unit->regs[0].source, RTLREG_UNDEFINED);
    rtl_make_unfoldable(unit, 0);
    EXPECT_EQ(unit->regs[0].source, RTLREG_UNDEFINED);
    EXPECT_STREQ(get_log_messages(),
                 "[error] rtl_make_unfoldable: Invalid register 0\n");
    clear_log_messages();

    EXPECT(unit->regs_size > 2);
    rtl_make_unfoldable(unit, 2);
    EXPECT_STREQ(get_log_messages(),
                 "[error] rtl_make_unfoldable: Invalid register 2\n");
    clear_log_messages();

    EXPECT_EQ(unit->regs[1].source, RTLREG_UNDEFINED);
    rtl_make_unfoldable(unit, 1);
    EXPECT_EQ(unit->regs[1].source, RTLREG_UNDEFINED);
    EXPECT_STREQ(get_log_messages(),
                 "[error] rtl_make_unfoldable: Register 1 has invalid"
                 " source undefined (must be constant or result)\n");
    clear_log_messages();

    rtl_add_insn(unit, RTLOP_LOAD_ARG, 1, 0, 0, 0);
    EXPECT_EQ(unit->regs[1].source, RTLREG_FUNC_ARG);
    rtl_make_unfoldable(unit, 1);
    EXPECT_EQ(unit->regs[1].source, RTLREG_FUNC_ARG);
    EXPECT_STREQ(get_log_messages(),
                 "[error] rtl_make_unfoldable: Register 1 has invalid"
                 " source argument (must be constant or result)\n");
    clear_log_messages();

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
