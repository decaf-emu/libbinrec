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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9, reg10, reg11;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg10 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg11 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg2, reg1, 0, RTLFEXC_INEXACT));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg3, reg1, 0, RTLFEXC_INVALID));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg4, reg1, 0, RTLFEXC_OVERFLOW));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg5, reg1, 0, RTLFEXC_UNDERFLOW));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg6, reg1, 0, RTLFEXC_ZERO_DIVIDE));
    EXPECT_EQ(unit->num_insns, 6);
    EXPECT_EQ(unit->insns[1].opcode, RTLOP_FTESTEXC);
    EXPECT_EQ(unit->insns[1].dest, reg2);
    EXPECT_EQ(unit->insns[1].src1, reg1);
    EXPECT_EQ(unit->insns[1].src_imm, RTLFEXC_INEXACT);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_FTESTEXC);
    EXPECT_EQ(unit->insns[2].dest, reg3);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[2].src_imm, RTLFEXC_INVALID);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_FTESTEXC);
    EXPECT_EQ(unit->insns[3].dest, reg4);
    EXPECT_EQ(unit->insns[3].src1, reg1);
    EXPECT_EQ(unit->insns[3].src_imm, RTLFEXC_OVERFLOW);
    EXPECT_EQ(unit->insns[4].opcode, RTLOP_FTESTEXC);
    EXPECT_EQ(unit->insns[4].dest, reg5);
    EXPECT_EQ(unit->insns[4].src1, reg1);
    EXPECT_EQ(unit->insns[4].src_imm, RTLFEXC_UNDERFLOW);
    EXPECT_EQ(unit->insns[5].opcode, RTLOP_FTESTEXC);
    EXPECT_EQ(unit->insns[5].dest, reg6);
    EXPECT_EQ(unit->insns[5].src1, reg1);
    EXPECT_EQ(unit->insns[5].src_imm, RTLFEXC_ZERO_DIVIDE);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 5);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 1);
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

    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg7, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg8, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg9, reg4, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg10, reg5, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg11, reg6, 0, 0));
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: FGETSTATE  r1\n"
        "    1: FTESTEXC   r2, r1, INEXACT\n"
        "           r1: fgetstate()\n"
        "    2: FTESTEXC   r3, r1, INVALID\n"
        "           r1: fgetstate()\n"
        "    3: FTESTEXC   r4, r1, OVERFLOW\n"
        "           r1: fgetstate()\n"
        "    4: FTESTEXC   r5, r1, UNDERFLOW\n"
        "           r1: fgetstate()\n"
        "    5: FTESTEXC   r6, r1, ZERO_DIVIDE\n"
        "           r1: fgetstate()\n"
        "    6: MOVE       r7, r2\n"
        "           r2: ftestexc(r1, INEXACT)\n"
        "    7: MOVE       r8, r3\n"
        "           r3: ftestexc(r1, INVALID)\n"
        "    8: MOVE       r9, r4\n"
        "           r4: ftestexc(r1, OVERFLOW)\n"
        "    9: MOVE       r10, r5\n"
        "           r5: ftestexc(r1, UNDERFLOW)\n"
        "   10: MOVE       r11, r6\n"
        "           r6: ftestexc(r1, ZERO_DIVIDE)\n"
        "\n"
        "Block 0: <none> --> [0,10] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
