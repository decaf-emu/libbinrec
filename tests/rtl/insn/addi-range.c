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
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, (int32_t)-0x80000000));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg3, reg1, 0, 0x7FFFFFFF));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg4, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg5, reg3, 0, 0));
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT_EQ(unit->insns[1].opcode, RTLOP_ADDI);
    EXPECT_EQ(unit->insns[1].dest, reg2);
    EXPECT_EQ(unit->insns[1].src1, reg1);
    EXPECT_EQ(unit->insns[1].src_imm, UINT64_C(0xFFFFFFFF80000000));
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_ADDI);
    EXPECT_EQ(unit->insns[2].dest, reg3);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[2].src_imm, 0x7FFFFFFF);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_ADDI, reg6, reg1, 0,
                              UINT64_C(-0x80000001)));
    EXPECT_ICE("Operand constraint violated:"
               " other + UINT64_C(0x80000000) < UINT64_C(0x100000000)");
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_ADDI, reg6, reg1, 0,
                              UINT64_C(0x80000000)));
    EXPECT_ICE("Operand constraint violated:"
               " other + UINT64_C(0x80000000) < UINT64_C(0x100000000)");
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT(unit->error);
    unit->error = false;
#endif

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 10\n"
        "    1: ADDI       r2, r1, -2147483648\n"
        "           r1: 10\n"
        "    2: ADDI       r3, r1, 2147483647\n"
        "           r1: 10\n"
        "    3: MOVE       r4, r2\n"
        "           r2: r1 + -2147483648\n"
        "    4: MOVE       r5, r3\n"
        "           r3: r1 + 2147483647\n"
        "\n"
        "Block 0: <none> --> [0,4] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
