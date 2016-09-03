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

    unit->regs_size = REGS_LIMIT - 1;
    unit->next_reg = unit->regs_size;
    EXPECT_EQ(rtl_alloc_register(unit, RTLTYPE_INT32), REGS_LIMIT - 1);
    EXPECT_EQ(unit->regs_size, REGS_LIMIT);
    EXPECT_EQ(unit->next_reg, REGS_LIMIT);
    EXPECT_EQ(unit->regs[REGS_LIMIT - 1].type, RTLTYPE_INT32);
    EXPECT_EQ(unit->regs[REGS_LIMIT - 1].source, RTLREG_UNDEFINED);
    EXPECT_FALSE(unit->regs[REGS_LIMIT - 1].live);

    EXPECT_EQ(rtl_alloc_register(unit, RTLTYPE_ADDRESS), 0);
    EXPECT_EQ(unit->regs_size, REGS_LIMIT);
    EXPECT_EQ(unit->next_reg, REGS_LIMIT);
    EXPECT_EQ(unit->regs[REGS_LIMIT - 1].type, RTLTYPE_INT32);
    EXPECT_EQ(unit->regs[REGS_LIMIT - 1].source, RTLREG_UNDEFINED);
    EXPECT_FALSE(unit->regs[REGS_LIMIT - 1].live);

    char expected_log[100];
    snprintf(expected_log, sizeof(expected_log),
             "[error] Too many registers in unit (limit %u)\n",
             REGS_LIMIT);
    EXPECT_STREQ(get_log_messages(), expected_log);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
