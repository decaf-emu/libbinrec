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
    EXPECT_EQ(unit->next_reg, 2);
    EXPECT_EQ(unit->regs[1].type, RTLTYPE_INT32);
    EXPECT_EQ(unit->regs[1].source, RTLREG_UNDEFINED);
    EXPECT_FALSE(unit->regs[1].live);

    /* Check behavior when the register array needs to be expanded. */
    unit->regs_size = 2;
    EXPECT_EQ(rtl_alloc_register(unit, RTLTYPE_ADDRESS), 2);
    EXPECT(unit->regs_size > 2);
    EXPECT_EQ(unit->next_reg, 3);
    EXPECT_EQ(unit->regs[1].type, RTLTYPE_INT32);
    EXPECT_EQ(unit->regs[1].source, RTLREG_UNDEFINED);
    EXPECT_FALSE(unit->regs[1].live);
    EXPECT_EQ(unit->regs[2].type, RTLTYPE_ADDRESS);
    EXPECT_EQ(unit->regs[2].source, RTLREG_UNDEFINED);
    EXPECT_FALSE(unit->regs[2].live);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
