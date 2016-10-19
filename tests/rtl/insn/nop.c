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
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, reg3, reg1, reg2,
                        UINT64_C(0x123456789)));
    EXPECT_EQ(unit->num_insns, 6);
    EXPECT_EQ(unit->insns[0].opcode, RTLOP_NOP);
    EXPECT_EQ(unit->insns[0].dest, 0);
    EXPECT_EQ(unit->insns[0].src1, 0);
    EXPECT_EQ(unit->insns[0].src2, 0);
    EXPECT_EQ(unit->insns[0].src_imm, 0);
    EXPECT_EQ(unit->insns[1].opcode, RTLOP_NOP);
    EXPECT_EQ(unit->insns[1].dest, 0);
    EXPECT_EQ(unit->insns[1].src1, 0);
    EXPECT_EQ(unit->insns[1].src2, 0);
    EXPECT_EQ(unit->insns[1].src_imm, 1);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_NOP);
    EXPECT_EQ(unit->insns[3].dest, 0);
    EXPECT_EQ(unit->insns[3].src1, reg1);
    EXPECT_EQ(unit->insns[3].src2, 0);
    EXPECT_EQ(unit->insns[3].src_imm, 0);
    EXPECT_EQ(unit->insns[5].opcode, RTLOP_NOP);
    EXPECT_EQ(unit->insns[5].dest, reg3);
    EXPECT_EQ(unit->insns[5].src1, reg1);
    EXPECT_EQ(unit->insns[5].src2, reg2);
    EXPECT_EQ(unit->insns[5].src_imm, UINT64_C(0x123456789));
    EXPECT_EQ(unit->regs[reg1].birth, 2);
    EXPECT_EQ(unit->regs[reg1].death, 5);
    EXPECT_EQ(unit->regs[reg2].birth, 4);
    EXPECT_EQ(unit->regs[reg2].death, 5);
    EXPECT_EQ(unit->regs[reg3].birth, 5);
    EXPECT_EQ(unit->regs[reg3].death, 5);
    EXPECT_EQ(unit->regs[reg3].source, RTLREG_RESULT_NOFOLD);
    EXPECT_EQ(unit->regs[reg3].result.opcode, RTLOP_NOP);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: NOP\n"
        "    1: NOP        -, 0x1\n"
        "    2: LOAD_IMM   r1, 0\n"
        "    3: NOP        -, r1\n"
        "    4: LOAD_IMM   r2, 0x0\n"
        "    5: NOP        r3, r1, r2, 0x123456789\n"
        "\n"
        "Block 0: <none> --> [0,5] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
