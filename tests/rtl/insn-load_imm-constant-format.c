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
        "\n"
        "Block    0: <none> --> [0,9] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
