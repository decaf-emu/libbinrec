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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0x3F800000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x40400000));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg4, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg5, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_VBROADCAST, reg6, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FMADD, reg7, reg4, reg5, reg6));
    EXPECT_EQ(unit->num_insns, 7);
    EXPECT_EQ(unit->insns[6].opcode, RTLOP_FMADD);
    EXPECT_EQ(unit->insns[6].dest, reg7);
    EXPECT_EQ(unit->insns[6].src1, reg4);
    EXPECT_EQ(unit->insns[6].src2, reg5);
    EXPECT_EQ(unit->insns[6].src3, reg6);
    EXPECT_EQ(unit->regs[reg4].birth, 3);
    EXPECT_EQ(unit->regs[reg4].death, 6);
    EXPECT_EQ(unit->regs[reg5].birth, 4);
    EXPECT_EQ(unit->regs[reg5].death, 6);
    EXPECT_EQ(unit->regs[reg6].birth, 5);
    EXPECT_EQ(unit->regs[reg6].death, 6);
    EXPECT_EQ(unit->regs[reg7].birth, 6);
    EXPECT_EQ(unit->regs[reg7].death, 6);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg8, reg7, 0, 0));
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 1.0f\n"
        "    1: LOAD_IMM   r2, 2.0f\n"
        "    2: LOAD_IMM   r3, 3.0f\n"
        "    3: VBROADCAST r4, r1\n"
        "           r1: 1.0f\n"
        "    4: VBROADCAST r5, r2\n"
        "           r2: 2.0f\n"
        "    5: VBROADCAST r6, r3\n"
        "           r3: 3.0f\n"
        "    6: FMADD      r7, r4, r5, r6\n"
        "           r4: vbroadcast(r1)\n"
        "           r5: vbroadcast(r2)\n"
        "           r6: vbroadcast(r3)\n"
        "    7: MOVE       r8, r7\n"
        "           r7: fma(r4, r5, r6)\n"
        "\n"
        "Block 0: <none> --> [0,7] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
