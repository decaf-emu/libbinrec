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

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));

    EXPECT(rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, reg1, 0, 0));
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_CALL_ADDR);
    EXPECT_EQ(unit->insns[2].dest, 0);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[2].src2, 0);
    EXPECT_EQ(unit->insns[2].src3, 0);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 2);

    EXPECT(rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, reg1, reg2, 0));
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_CALL_ADDR);
    EXPECT_EQ(unit->insns[3].dest, 0);
    EXPECT_EQ(unit->insns[3].src1, reg1);
    EXPECT_EQ(unit->insns[3].src2, reg2);
    EXPECT_EQ(unit->insns[3].src3, 0);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 3);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 3);

    EXPECT(rtl_add_insn(unit, RTLOP_CALL_ADDR, reg3, reg1, 0, 0));
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT_EQ(unit->insns[4].opcode, RTLOP_CALL_ADDR);
    EXPECT_EQ(unit->insns[4].dest, reg3);
    EXPECT_EQ(unit->insns[4].src1, reg1);
    EXPECT_EQ(unit->insns[4].src2, 0);
    EXPECT_EQ(unit->insns[4].src3, 0);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 4);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 3);
    EXPECT_EQ(unit->regs[reg3].birth, 4);
    EXPECT_EQ(unit->regs[reg3].death, 4);
    EXPECT_EQ(unit->regs[reg3].source, RTLREG_RESULT_NOFOLD);
    EXPECT_EQ(unit->regs[reg3].result.opcode, RTLOP_CALL_ADDR);

    EXPECT(rtl_add_insn(unit, RTLOP_CALL_ADDR, 0, reg1, reg2, reg3));
    EXPECT_EQ(unit->num_insns, 6);
    EXPECT_EQ(unit->insns[5].opcode, RTLOP_CALL_ADDR);
    EXPECT_EQ(unit->insns[5].dest, 0);
    EXPECT_EQ(unit->insns[5].src1, reg1);
    EXPECT_EQ(unit->insns[5].src2, reg2);
    EXPECT_EQ(unit->insns[5].src3, reg3);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 5);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 5);
    EXPECT_EQ(unit->regs[reg3].birth, 4);
    EXPECT_EQ(unit->regs[reg3].death, 5);

    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 0xA\n"
        "    1: LOAD_IMM   r2, 20\n"
        "    2: CALL_ADDR  @r1\n"
        "           r1: 0xA\n"
        "    3: CALL_ADDR  @r1, r2\n"
        "           r1: 0xA\n"
        "           r2: 20\n"
        "    4: CALL_ADDR  r3, @r1\n"
        "           r1: 0xA\n"
        "    5: CALL_ADDR  @r1, r2, r3\n"
        "           r1: 0xA\n"
        "           r2: 20\n"
        "           r3: call(...)\n"
        "\n"
        "Block 0: <none> --> [0,5] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
