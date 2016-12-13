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

    int reg1, reg2, alias1, alias2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_V2_FLOAT32));

    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg1, 0, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias2));

    int insn = 2;
    for (int ordered = 0; ordered <= 1; ordered++) {
        for (int invert = 0; invert <= 1; invert++) {
            for (int cmp = RTLFCMP_LT; cmp <= RTLFCMP_UN; cmp++, insn += 2) {
                const int fcmp = (ordered ? RTLFCMP_ORDERED : 0)
                               | (invert ? RTLFCMP_INVERT : 0)
                               | cmp;
                int regA, regB;
                EXPECT(regA = rtl_alloc_register(unit, RTLTYPE_INT64));
                EXPECT(regB = rtl_alloc_register(unit, RTLTYPE_INT64));
                if (!rtl_add_insn(unit, RTLOP_VFCMP, regA, reg1, reg2, fcmp)) {
                    FAIL("rtl_add_insn(unit, RTLOP_VFCMP, regA, reg1, reg2,"
                         " fcmp) was not true as expected for fcmp=0x%X",
                         fcmp);
                }
                EXPECT_EQ(unit->num_insns, insn+1);
                EXPECT_EQ(unit->insns[insn].opcode, RTLOP_VFCMP);
                EXPECT_EQ(unit->insns[insn].dest, regA);
                EXPECT_EQ(unit->insns[insn].src1, reg1);
                EXPECT_EQ(unit->insns[insn].src2, reg2);
                EXPECT_EQ(unit->insns[insn].fcmp, fcmp);
                EXPECT_EQ(unit->regs[reg1].birth, 0);
                EXPECT_EQ(unit->regs[reg1].death, insn);
                EXPECT_EQ(unit->regs[reg2].birth, 1);
                EXPECT_EQ(unit->regs[reg2].death, insn);
                EXPECT_EQ(unit->regs[regA].birth, insn);
                EXPECT_EQ(unit->regs[regA].death, insn);
                EXPECT(unit->have_block);
                EXPECT_FALSE(unit->error);

                EXPECT(rtl_add_insn(unit, RTLOP_MOVE, regB, regA, 0, 0));
                EXPECT_FALSE(unit->error);
            }
        }
    }

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: GET_ALIAS  r1, a1\n"
        "    1: GET_ALIAS  r2, a2\n"
        "    2: VFCMP      r3, r1, r2, LT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "    3: MOVE       r4, r3\n"
        "           r3: vfcmp(r1, r2, LT)\n"
        "    4: VFCMP      r5, r1, r2, LE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "    5: MOVE       r6, r5\n"
        "           r5: vfcmp(r1, r2, LE)\n"
        "    6: VFCMP      r7, r1, r2, GT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "    7: MOVE       r8, r7\n"
        "           r7: vfcmp(r1, r2, GT)\n"
        "    8: VFCMP      r9, r1, r2, GE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "    9: MOVE       r10, r9\n"
        "           r9: vfcmp(r1, r2, GE)\n"
        "   10: VFCMP      r11, r1, r2, EQ\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   11: MOVE       r12, r11\n"
        "           r11: vfcmp(r1, r2, EQ)\n"
        "   12: VFCMP      r13, r1, r2, UN\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   13: MOVE       r14, r13\n"
        "           r13: vfcmp(r1, r2, UN)\n"
        "   14: VFCMP      r15, r1, r2, NLT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   15: MOVE       r16, r15\n"
        "           r15: vfcmp(r1, r2, NLT)\n"
        "   16: VFCMP      r17, r1, r2, NLE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   17: MOVE       r18, r17\n"
        "           r17: vfcmp(r1, r2, NLE)\n"
        "   18: VFCMP      r19, r1, r2, NGT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   19: MOVE       r20, r19\n"
        "           r19: vfcmp(r1, r2, NGT)\n"
        "   20: VFCMP      r21, r1, r2, NGE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   21: MOVE       r22, r21\n"
        "           r21: vfcmp(r1, r2, NGE)\n"
        "   22: VFCMP      r23, r1, r2, NEQ\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   23: MOVE       r24, r23\n"
        "           r23: vfcmp(r1, r2, NEQ)\n"
        "   24: VFCMP      r25, r1, r2, NUN\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   25: MOVE       r26, r25\n"
        "           r25: vfcmp(r1, r2, NUN)\n"
        "   26: VFCMP      r27, r1, r2, OLT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   27: MOVE       r28, r27\n"
        "           r27: vfcmp(r1, r2, OLT)\n"
        "   28: VFCMP      r29, r1, r2, OLE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   29: MOVE       r30, r29\n"
        "           r29: vfcmp(r1, r2, OLE)\n"
        "   30: VFCMP      r31, r1, r2, OGT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   31: MOVE       r32, r31\n"
        "           r31: vfcmp(r1, r2, OGT)\n"
        "   32: VFCMP      r33, r1, r2, OGE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   33: MOVE       r34, r33\n"
        "           r33: vfcmp(r1, r2, OGE)\n"
        "   34: VFCMP      r35, r1, r2, OEQ\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   35: MOVE       r36, r35\n"
        "           r35: vfcmp(r1, r2, OEQ)\n"
        "   36: VFCMP      r37, r1, r2, OUN\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   37: MOVE       r38, r37\n"
        "           r37: vfcmp(r1, r2, OUN)\n"
        "   38: VFCMP      r39, r1, r2, ONLT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   39: MOVE       r40, r39\n"
        "           r39: vfcmp(r1, r2, ONLT)\n"
        "   40: VFCMP      r41, r1, r2, ONLE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   41: MOVE       r42, r41\n"
        "           r41: vfcmp(r1, r2, ONLE)\n"
        "   42: VFCMP      r43, r1, r2, ONGT\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   43: MOVE       r44, r43\n"
        "           r43: vfcmp(r1, r2, ONGT)\n"
        "   44: VFCMP      r45, r1, r2, ONGE\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   45: MOVE       r46, r45\n"
        "           r45: vfcmp(r1, r2, ONGE)\n"
        "   46: VFCMP      r47, r1, r2, ONEQ\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   47: MOVE       r48, r47\n"
        "           r47: vfcmp(r1, r2, ONEQ)\n"
        "   48: VFCMP      r49, r1, r2, ONUN\n"
        "           r1: a1\n"
        "           r2: a2\n"
        "   49: MOVE       r50, r49\n"
        "           r49: vfcmp(r1, r2, ONUN)\n"
        "\n"
        "Alias 1: float32[2], no bound storage\n"
        "Alias 2: float32[2], no bound storage\n"
        "\n"
        "Block 0: <none> --> [0,49] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
