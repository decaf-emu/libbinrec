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

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT_EQ(unit->regs[reg1].source, RTLREG_CONSTANT);
    rtl_make_unfoldable(unit, reg1);
    EXPECT_EQ(unit->regs[reg1].source, RTLREG_CONSTANT_NOFOLD);
    EXPECT_STREQ(get_log_messages(), NULL);

    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 1));
    EXPECT_EQ(unit->regs[reg2].source, RTLREG_RESULT);
    rtl_make_unfoldable(unit, reg2);
    EXPECT_EQ(unit->regs[reg2].source, RTLREG_RESULT_NOFOLD);
    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
