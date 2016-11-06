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

    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));

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

    /* 32-bit operations should ignore the high bits of the immediate. */
    int reg6, reg7, reg8, reg9, reg10;
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg10 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg6, 0, 0, 60));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI,
                        reg7, reg6, 0, UINT64_C(0x1234567800000001)));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI,
                        reg8, reg6, 0, UINT64_C(0x12345678FFFFFFFF)));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg9, reg7, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg10, reg8, 0, 0));
    EXPECT_EQ(unit->num_insns, 10);
    EXPECT_EQ(unit->insns[6].opcode, RTLOP_ADDI);
    EXPECT_EQ(unit->insns[6].dest, reg7);
    EXPECT_EQ(unit->insns[6].src1, reg6);
    EXPECT_EQ(unit->insns[6].src_imm, 1);
    EXPECT_EQ(unit->insns[7].opcode, RTLOP_ADDI);
    EXPECT_EQ(unit->insns[7].dest, reg8);
    EXPECT_EQ(unit->insns[7].src1, reg6);
    EXPECT_EQ(unit->insns[7].src_imm, UINT64_C(-1));
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    int reg11;
    EXPECT(reg11 = rtl_alloc_register(unit, RTLTYPE_INT64));

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_ADDI, reg11, reg1, 0,
                              UINT64_C(-0x80000001)));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[dest].type == RTLTYPE_INT32 ||"
               " other + UINT64_C(0x80000000) < UINT64_C(0x100000000)");
    EXPECT_EQ(unit->num_insns, 10);
    EXPECT(unit->error);
    unit->error = false;

    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_ADDI, reg11, reg1, 0,
                              UINT64_C(0x80000000)));
    EXPECT_ICE("Operand constraint violated:"
               " unit->regs[dest].type == RTLTYPE_INT32 ||"
               " other + UINT64_C(0x80000000) < UINT64_C(0x100000000)");
    EXPECT_EQ(unit->num_insns, 10);
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
        "    5: LOAD_IMM   r6, 60\n"
        "    6: ADDI       r7, r6, 1\n"
        "           r6: 60\n"
        "    7: ADDI       r8, r6, -1\n"
        "           r6: 60\n"
        "    8: MOVE       r9, r7\n"
        "           r7: r6 + 1\n"
        "    9: MOVE       r10, r8\n"
        "           r8: r6 + -1\n"
        "\n"
        "Block 0: <none> --> [0,9] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
