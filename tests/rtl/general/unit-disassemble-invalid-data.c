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

    int reg1, reg2, reg3, reg4;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg3, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg4, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    /* Set invalid opcodes and register sources.  These cases will never
     * be encountered in ordinary usage; this test is just to exercise the
     * default cases in various switch blocks. */
    unit->insns[1].opcode = (RTLOpcode)0;
    unit->insns[4].src2 = reg4;
    unit->regs[reg2].result.opcode = (RTLOpcode)0;
    unit->regs[reg3].source = (RTLRegType)255;

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, true));
    EXPECT_STREQ(disassembly,
                 "    0: LOAD_IMM   r1, 10\n"
                 "    1: <invalid opcode 0>\n"
                 "    2: MOVE       r3, r2\n"
                 "           r2: <invalid result opcode 0>\n"
                 "    3: MOVE       r4, r3\n"
                 "           r3: <invalid register source 255>\n"
                 "    4: NOP        -, <missing operand>, r4\n"
                 "\n"
                 "Block 0: <none> --> [0,4] --> <none>\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
