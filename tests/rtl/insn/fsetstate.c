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

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));

    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETSTATE, 0, reg1, 0, 0));
    EXPECT_EQ(unit->num_insns, 2);
    EXPECT_EQ(unit->insns[1].opcode, RTLOP_FSETSTATE);
    EXPECT_EQ(unit->insns[1].src1, reg1);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: FGETSTATE  r1\n"
        "    1: FSETSTATE  r1\n"
        "           r1: fgetstate()\n"
        "\n"
        "Block 0: <none> --> [0,1] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
