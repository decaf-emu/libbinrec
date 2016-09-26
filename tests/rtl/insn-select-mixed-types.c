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

    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 30));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 50));
    /* This should be allowed since the condition type does not need to
     * match the data type being operated on. */
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg4, reg1, reg2, reg3));
    /* Also check that a condition type of ADDRESS is allowed. */
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg6, reg3, reg5, reg4));
    EXPECT_EQ(unit->num_insns, 6);
    EXPECT_EQ(unit->insns[4].opcode, RTLOP_SELECT);
    EXPECT_EQ(unit->insns[4].dest, reg4);
    EXPECT_EQ(unit->insns[4].src1, reg1);
    EXPECT_EQ(unit->insns[4].src2, reg2);
    EXPECT_EQ(unit->insns[4].cond, reg3);
    EXPECT_EQ(unit->insns[5].opcode, RTLOP_SELECT);
    EXPECT_EQ(unit->insns[5].dest, reg6);
    EXPECT_EQ(unit->insns[5].src1, reg3);
    EXPECT_EQ(unit->insns[5].src2, reg5);
    EXPECT_EQ(unit->insns[5].cond, reg4);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 0xA\n"
        "    1: LOAD_IMM   r2, 0x14\n"
        "    2: LOAD_IMM   r3, 30\n"
        "    3: LOAD_IMM   r5, 50\n"
        "    4: SELECT     r4, r1, r2, r3\n"
        "           r1: 0xA\n"
        "           r2: 0x14\n"
        "           r3: 30\n"
        "    5: SELECT     r6, r3, r5, r4\n"
        "           r3: 30\n"
        "           r5: 50\n"
        "           r4: r3 ? r1 : r2\n"
        "\n"
        "Block 0: <none> --> [0,5] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
