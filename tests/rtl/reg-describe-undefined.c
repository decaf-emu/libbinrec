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


int main(void)
{
    /* If operand sanity checks are enabled, rtl.c:rtl_describe_register()
     * will never see an RTLREG_UNDEFINED register, so we feed it one here
     * just to test the code path. */

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    uint32_t reg;
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    unit->insns[0].src1 = reg;

    const char *output = rtl_disassemble_unit(unit, true);
    EXPECT_STREQ(output, ("    0: RETURN     r1\n"
                          "           r1: (undefined)\n"
                          "\n"
                          "Block    0: <none> --> [0,0] --> <none>\n"));

    rtl_destroy_unit(unit);
    return EXIT_SUCCESS;
}
