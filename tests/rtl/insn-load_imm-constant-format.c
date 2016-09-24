/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/bitutils.h"
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

    /* Check that small constants in INT32 registers are disassembled to
     * decimal, but ADDRESS registers always use hex. */

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9, reg10;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg10 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0xFFFF));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0xFFFF8000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0x10000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0xFFFF7FFF));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg6, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg7, reg2, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg8, reg3, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg9, reg4, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg10, reg5, 0, 0));

    /* Check that floating-point constants have the expected format and
     * number of significant digits. */
    int reg11, reg12, reg13, reg14, reg15, reg16, reg17, reg18, reg19, reg20;
    int reg21, reg22, reg23, reg24, reg25, reg26, reg27, reg28, reg29, reg30;
    EXPECT(reg11 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg12 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg13 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg14 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg15 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg16 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg17 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg18 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg19 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg20 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(reg21 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg22 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg23 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg24 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg25 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg26 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg27 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg28 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg29 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));
    EXPECT(reg30 = rtl_alloc_register(unit, RTLTYPE_DOUBLE));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg11, 0, 0, 0x3F800000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg12, 0, 0, 0x3FAAAAAA));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg13, 0, 0, 0x7F800000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg14, 0, 0, 0xFF800000));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg15, 0, 0, 0x7F800001));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg16, reg11, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg17, reg12, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg18, reg13, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg19, reg14, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg20, reg15, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg21, 0, 0,
                        UINT64_C(0x3FF0000000000000)));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg22, 0, 0,
                        UINT64_C(0x3FF5555555555555)));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg23, 0, 0,
                        UINT64_C(0x7FF0000000000000)));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg24, 0, 0,
                        UINT64_C(0xFFF0000000000000)));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg25, 0, 0,
                        UINT64_C(0x7FF0000000000001)));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg26, reg21, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg27, reg22, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg28, reg23, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg29, reg24, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_MOVE, reg30, reg25, 0, 0));

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 65535\n"
        "    1: LOAD_IMM   r2, -32768\n"
        "    2: LOAD_IMM   r3, 0x10000\n"
        "    3: LOAD_IMM   r4, 0xFFFF7FFF\n"
        "    4: LOAD_IMM   r5, 0x1\n"
        "    5: MOVE       r6, r1\n"
        "           r1: 65535\n"
        "    6: MOVE       r7, r2\n"
        "           r2: -32768\n"
        "    7: MOVE       r8, r3\n"
        "           r3: 0x10000\n"
        "    8: MOVE       r9, r4\n"
        "           r4: 0xFFFF7FFF\n"
        "    9: MOVE       r10, r5\n"
        "           r5: 0x1\n"
        "   10: LOAD_IMM   r11, 1.0f\n"
        "   11: LOAD_IMM   r12, 1.3333333f\n"
        "   12: LOAD_IMM   r13, inf\n"
        "   13: LOAD_IMM   r14, -inf\n"
        "   14: LOAD_IMM   r15, nan(0x1)\n"
        "   15: MOVE       r16, r11\n"
        "           r11: 1.0f\n"
        "   16: MOVE       r17, r12\n"
        "           r12: 1.3333333f\n"
        "   17: MOVE       r18, r13\n"
        "           r13: inf\n"
        "   18: MOVE       r19, r14\n"
        "           r14: -inf\n"
        "   19: MOVE       r20, r15\n"
        "           r15: nan(0x1)\n"
        "   20: LOAD_IMM   r21, 1.0\n"
        "   21: LOAD_IMM   r22, 1.3333333333333333\n"
        "   22: LOAD_IMM   r23, inf\n"
        "   23: LOAD_IMM   r24, -inf\n"
        "   24: LOAD_IMM   r25, nan(0x1)\n"
        "   25: MOVE       r26, r21\n"
        "           r21: 1.0\n"
        "   26: MOVE       r27, r22\n"
        "           r22: 1.3333333333333333\n"
        "   27: MOVE       r28, r23\n"
        "           r23: inf\n"
        "   28: MOVE       r29, r24\n"
        "           r24: -inf\n"
        "   29: MOVE       r30, r25\n"
        "           r25: nan(0x1)\n"
        "\n"
        "Block    0: <none> --> [0,29] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
