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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_FSCAST, reg3, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FSCAST, reg4, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FSCAST, reg5, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FSCAST, reg6, reg2, 0, 0));
    EXPECT_EQ(unit->num_insns, 6);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_FSCAST);
    EXPECT_EQ(unit->insns[2].dest, reg3);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_FSCAST);
    EXPECT_EQ(unit->insns[3].dest, reg4);
    EXPECT_EQ(unit->insns[3].src1, reg2);
    EXPECT_EQ(unit->insns[4].opcode, RTLOP_FSCAST);
    EXPECT_EQ(unit->insns[4].dest, reg5);
    EXPECT_EQ(unit->insns[4].src1, reg1);
    EXPECT_EQ(unit->insns[5].opcode, RTLOP_FSCAST);
    EXPECT_EQ(unit->insns[5].dest, reg6);
    EXPECT_EQ(unit->insns[5].src1, reg2);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 4);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 5);
    EXPECT_EQ(unit->regs[reg3].birth, 2);
    EXPECT_EQ(unit->regs[reg3].death, 2);
    EXPECT_EQ(unit->regs[reg4].birth, 3);
    EXPECT_EQ(unit->regs[reg4].death, 3);
    EXPECT_EQ(unit->regs[reg5].birth, 4);
    EXPECT_EQ(unit->regs[reg5].death, 4);
    EXPECT_EQ(unit->regs[reg6].birth, 5);
    EXPECT_EQ(unit->regs[reg6].death, 5);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg7, reg3, 0, 0));
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 10\n"
        "    1: LOAD_IMM   r2, 20\n"
        "    2: FSCAST     r3, r1\n"
        "           r1: 10\n"
        "    3: FSCAST     r4, r2\n"
        "           r2: 20\n"
        "    4: FSCAST     r5, r1\n"
        "           r1: 10\n"
        "    5: FSCAST     r6, r2\n"
        "           r2: 20\n"
        "    6: MOVE       r7, r3\n"
        "           r3: fscast(r1)\n"
        "\n"
        "Block 0: <none> --> [0,6] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
