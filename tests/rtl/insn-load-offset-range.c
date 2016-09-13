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

    uint32_t reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg3, reg1, 0, 0x7FFF));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg4, reg1, 0, -0x8000));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg5, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg6, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg7, reg4, 0, 0));
    EXPECT_EQ(unit->num_insns, 7);
    EXPECT_EQ(unit->insns[1].opcode, RTLOP_LOAD);
    EXPECT_EQ(unit->insns[1].dest, reg2);
    EXPECT_EQ(unit->insns[1].src1, reg1);
    EXPECT_EQ(unit->insns[1].offset, 0);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_LOAD);
    EXPECT_EQ(unit->insns[2].dest, reg3);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[2].offset, 0x7FFF);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_LOAD);
    EXPECT_EQ(unit->insns[3].dest, reg4);
    EXPECT_EQ(unit->insns[3].src1, reg1);
    EXPECT_EQ(unit->insns[3].offset, -0x8000);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_LOAD_U8, reg8, reg1, 0, 0x8000));
    EXPECT_ICE("Operand constraint violated:"
               " other <= 0x7FFF || other >= UINT64_C(-0x8000)");
    EXPECT_EQ(unit->num_insns, 7);
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_LOAD_U8, reg8, reg1, 0, -0x8001));
    EXPECT_ICE("Operand constraint violated:"
               " other <= 0x7FFF || other >= UINT64_C(-0x8000)");
    EXPECT_EQ(unit->num_insns, 7);
#endif

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 0xA\n"
        "    1: LOAD       r2, 0(r1)\n"
        "           r1: 0xA\n"
        "    2: LOAD       r3, 32767(r1)\n"
        "           r1: 0xA\n"
        "    3: LOAD       r4, -32768(r1)\n"
        "           r1: 0xA\n"
        "    4: MOVE       r5, r2\n"
        "           r2: @0(r1).i32\n"
        "    5: MOVE       r6, r3\n"
        "           r3: @32767(r1).i32\n"
        "    6: MOVE       r7, r4\n"
        "           r4: @-32768(r1).i32\n"
        "\n"
        "Block    0: <none> --> [0,6] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
