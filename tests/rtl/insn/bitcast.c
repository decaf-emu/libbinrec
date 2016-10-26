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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg4, 0, 0, UINT64_C(0x4010000000000000)));
    EXPECT(rtl_add_insn(unit, RTLOP_BITCAST, reg5, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_BITCAST, reg6, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_BITCAST, reg7, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_BITCAST, reg8, reg4, 0, 0));
    EXPECT_EQ(unit->num_insns, 8);
    EXPECT_EQ(unit->insns[4].opcode, RTLOP_BITCAST);
    EXPECT_EQ(unit->insns[4].dest, reg5);
    EXPECT_EQ(unit->insns[4].src1, reg1);
    EXPECT_EQ(unit->insns[5].opcode, RTLOP_BITCAST);
    EXPECT_EQ(unit->insns[5].dest, reg6);
    EXPECT_EQ(unit->insns[5].src1, reg2);
    EXPECT_EQ(unit->insns[6].opcode, RTLOP_BITCAST);
    EXPECT_EQ(unit->insns[6].dest, reg7);
    EXPECT_EQ(unit->insns[6].src1, reg3);
    EXPECT_EQ(unit->insns[7].opcode, RTLOP_BITCAST);
    EXPECT_EQ(unit->insns[7].dest, reg8);
    EXPECT_EQ(unit->insns[7].src1, reg4);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 4);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 5);
    EXPECT_EQ(unit->regs[reg3].birth, 2);
    EXPECT_EQ(unit->regs[reg3].death, 6);
    EXPECT_EQ(unit->regs[reg4].birth, 3);
    EXPECT_EQ(unit->regs[reg4].death, 7);
    EXPECT_EQ(unit->regs[reg5].birth, 4);
    EXPECT_EQ(unit->regs[reg5].death, 4);
    EXPECT_EQ(unit->regs[reg6].birth, 5);
    EXPECT_EQ(unit->regs[reg6].death, 5);
    EXPECT_EQ(unit->regs[reg7].birth, 6);
    EXPECT_EQ(unit->regs[reg7].death, 6);
    EXPECT_EQ(unit->regs[reg8].birth, 7);
    EXPECT_EQ(unit->regs[reg8].death, 7);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg9, reg5, 0, 0));
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 10\n"
        "    1: LOAD_IMM   r2, 20\n"
        "    2: LOAD_IMM   r3, 3.0f\n"
        "    3: LOAD_IMM   r4, 4.0\n"
        "    4: BITCAST    r5, r1\n"
        "           r1: 10\n"
        "    5: BITCAST    r6, r2\n"
        "           r2: 20\n"
        "    6: BITCAST    r7, r3\n"
        "           r3: 3.0f\n"
        "    7: BITCAST    r8, r4\n"
        "           r4: 4.0\n"
        "    8: MOVE       r9, r5\n"
        "           r5: bitcast(r1)\n"
        "\n"
        "Block 0: <none> --> [0,8] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
