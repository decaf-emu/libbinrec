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

    uint32_t reg1, reg2, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    ASSERT(reg2 != alias);

    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, alias, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg1, reg2, 0, 0));
    EXPECT_EQ(unit->num_insns, 2);
    EXPECT_EQ(unit->insns[0].opcode, RTLOP_GET_ALIAS);
    EXPECT_EQ(unit->insns[0].dest, reg2);
    EXPECT_EQ(unit->insns[0].src1, alias);
    EXPECT(unit->have_block);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: GET_ALIAS  r2, a1\n"
        "    1: MOVE       r1, r2\n"
        "           r2: a1\n"
        "\n"
        "Block    0: <none> --> [0,1] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
