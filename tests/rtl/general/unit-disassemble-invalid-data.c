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
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0x40000000));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg3, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg4, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg5, reg4, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg6, reg2, reg2, RTLFCMP_EQ));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg7, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC, reg8, reg7, 0, RTLFEXC_INVALID));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETROUND, reg9, reg7, 0, RTLFROUND_CEIL));
    /* Set invalid opcodes and register sources.  These cases will never
     * be encountered in ordinary usage; this test is just to exercise the
     * default cases in various switch blocks. */
    unit->insns[2].opcode = (RTLOpcode)0;
    unit->insns[5].src2 = reg5;
    unit->insns[6].fcmp = RTLFCMP_UN + 1;
    unit->insns[8].src_imm = RTLFEXC_ZERO_DIVIDE + 1;
    unit->insns[9].src_imm = RTLFROUND_CEIL + 1;
    unit->regs[reg3].result.opcode = (RTLOpcode)0;
    unit->regs[reg4].source = (RTLRegType)255;

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, true));
    EXPECT_STREQ(disassembly,
                 "    0: LOAD_IMM   r1, 10\n"
                 "    1: LOAD_IMM   r2, 2.0f\n"
                 "    2: <invalid opcode 0>\n"
                 "    3: MOVE       r4, r3\n"
                 "           r3: <invalid result opcode 0>\n"
                 "    4: MOVE       r5, r4\n"
                 "           r4: <invalid register source 255>\n"
                 "    5: NOP        -, <missing operand>, r5\n"
                 "    6: FCMP       r6, r2, r2, ???\n"
                 "           r2: 2.0f\n"
                 "           r2: 2.0f\n"
                 "    7: FGETSTATE  r7\n"
                 "    8: FTESTEXC   r8, r7, ???\n"
                 "           r7: fgetstate()\n"
                 "    9: FSETROUND  r9, r7, ???\n"
                 "           r7: fgetstate()\n"
                 "\n"
                 "Block 0: <none> --> [0,9] --> <none>\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
