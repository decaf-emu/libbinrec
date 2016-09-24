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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg8, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg4, reg1, reg2, reg3));
    EXPECT(rtl_add_insn(unit, RTLOP_BFEXT, reg5, reg4, 0, 2 | 4<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg6, reg4, reg5, 6 | 8<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg7, reg8, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg8, reg6, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg7, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg6, 0, 0));

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, false));
    EXPECT_STREQ(disassembly,
                 "    0: LABEL      L1\n"
                 "    1: LOAD_IMM   r1, 10\n"
                 "    2: LOAD_IMM   r8, 0x14\n"
                 "    3: MOVE       r2, r1\n"
                 "    4: ADD        r3, r1, r2\n"
                 "    5: SELECT     r4, r1, r2, r3\n"
                 "    6: BFEXT      r5, r4, 2, 4\n"
                 "    7: BFINS      r6, r4, r5, 6, 8\n"
                 "    8: LOAD       r7, 0(r8)\n"
                 "    9: STORE      0(r8), r6\n"
                 "   10: GOTO_IF_NZ r7, L1\n"
                 "   11: RETURN     r6\n"
                 "\n"
                 "Block    0: 0 --> [0,10] --> 1,0\n"
                 "Block    1: 0 --> [11,11] --> <none>\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
