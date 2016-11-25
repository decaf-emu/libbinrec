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
    EXPECT_EQ(unit->membase_reg, 0);

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    rtl_set_membase_pointer(unit, reg1);
    EXPECT_EQ(unit->membase_reg, reg1);

    /* CONSTANT and RESULT types should be made unfoldable. */
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    rtl_set_membase_pointer(unit, reg2);
    EXPECT_EQ(unit->membase_reg, reg2);
    EXPECT_EQ(unit->regs[reg2].source, RTLREG_CONSTANT_NOFOLD);

    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    rtl_set_membase_pointer(unit, reg3);
    EXPECT_EQ(unit->membase_reg, reg3);
    EXPECT_EQ(unit->regs[reg3].source, RTLREG_RESULT_NOFOLD);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
