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

    int reg1, reg2, reg3, reg4, reg5, alias1, alias2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT64));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT64));

    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg1, 0, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias2));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCVT, reg3, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_VFCVT, reg4, reg2, 0, 0));
    EXPECT_EQ(unit->num_insns, 4);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_VFCVT);
    EXPECT_EQ(unit->insns[2].dest, reg3);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_VFCVT);
    EXPECT_EQ(unit->insns[3].dest, reg4);
    EXPECT_EQ(unit->insns[3].src1, reg2);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 2);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 3);
    EXPECT_EQ(unit->regs[reg3].birth, 2);
    EXPECT_EQ(unit->regs[reg3].death, 2);
    EXPECT_EQ(unit->regs[reg4].birth, 3);
    EXPECT_EQ(unit->regs[reg4].death, 3);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg5, reg3, 0, 0));
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: GET_ALIAS  r1, a1\n"
        "    1: GET_ALIAS  r2, a2\n"
        "    2: VFCVT      r3, r1\n"
        "           r1: a1\n"
        "    3: VFCVT      r4, r2\n"
        "           r2: a2\n"
        "    4: MOVE       r5, r3\n"
        "           r3: vfcvt(r1)\n"
        "\n"
        "Alias 1: float32[2], no bound storage\n"
        "Alias 2: float64[2], no bound storage\n"
        "\n"
        "Block 0: <none> --> [0,4] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
