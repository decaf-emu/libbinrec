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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9, reg10, reg11;
    int reg12, reg13, reg14, reg15, reg16, reg17, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(reg10 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg11 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg12 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(reg13 = rtl_alloc_register(unit, RTLTYPE_V2_FLOAT32));
    EXPECT(reg14 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg15 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg16 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg17 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg14, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SELECT, reg4, reg1, reg2, reg3));
    EXPECT(rtl_add_insn(unit, RTLOP_BFEXT, reg5, reg4, 0, 2 | 4<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg6, reg4, reg5, 6 | 8<<8));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg7, 0, 0, 0x40400000));
    EXPECT(rtl_add_insn(unit, RTLOP_FCMP, reg8, reg7, reg7, RTLFCMP_EQ));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg9, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_FTESTEXC,
                        reg10, reg9, 0, RTLFEXC_INVALID));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg11, reg14, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_VEXTRACT, reg12, reg11, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_VINSERT, reg13, reg11, reg12, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg14, reg13, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_ATOMIC_INC, reg15, reg14, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_CMPXCHG, reg16, reg14, reg6, reg10));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg17, reg14, reg6, reg10));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg17, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg17, 0, 0));

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, false));
    EXPECT_STREQ(disassembly,
                 "    0: LABEL      L1\n"
                 "    1: LOAD_IMM   r1, 10\n"
                 "    2: LOAD_IMM   r14, 0x14\n"
                 "    3: MOVE       r2, r1\n"
                 "    4: ADD        r3, r1, r2\n"
                 "    5: SELECT     r4, r1, r2, r3\n"
                 "    6: BFEXT      r5, r4, 2, 4\n"
                 "    7: BFINS      r6, r4, r5, 6, 8\n"
                 "    8: LOAD_IMM   r7, 3.0f\n"
                 "    9: FCMP       r8, r7, r7, EQ\n"
                 "   10: FGETSTATE  r9\n"
                 "   11: FTESTEXC   r10, r9, INVALID\n"
                 "   12: LOAD       r11, 0(r14)\n"
                 "   13: VEXTRACT   r12, r11, 0\n"
                 "   14: VINSERT    r13, r11, r12, 1\n"
                 "   15: STORE      0(r14), r13\n"
                 "   16: ATOMIC_INC r15, (r14)\n"
                 "   17: CMPXCHG    r16, (r14), r6, r10\n"
                 "   18: CALL       r17, @r14, r6, r10\n"
                 "   19: GOTO_IF_NZ r17, L1\n"
                 "   20: RETURN     r17\n"
                 "\n"
                 "Block 0: 0 --> [0,19] --> 1,0\n"
                 "Block 1: 0 --> [20,20] --> <none>\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
