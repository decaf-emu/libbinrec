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

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_I8, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_I8, 0, reg1, reg2, 0x7FFF));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_I8, 0, reg1, reg2, -0x8000));
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_STORE_I8);
    EXPECT_EQ(unit->insns[2].src1, reg1);
    EXPECT_EQ(unit->insns[2].src2, reg2);
    EXPECT_EQ(unit->insns[2].offset, 0);
    EXPECT_EQ(unit->insns[3].opcode, RTLOP_STORE_I8);
    EXPECT_EQ(unit->insns[3].src1, reg1);
    EXPECT_EQ(unit->insns[3].src2, reg2);
    EXPECT_EQ(unit->insns[3].offset, 0x7FFF);
    EXPECT_EQ(unit->insns[4].opcode, RTLOP_STORE_I8);
    EXPECT_EQ(unit->insns[4].src1, reg1);
    EXPECT_EQ(unit->insns[4].src2, reg2);
    EXPECT_EQ(unit->insns[4].offset, -0x8000);
    EXPECT(unit->have_block);

#ifdef ENABLE_OPERAND_SANITY_CHECKS
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_STORE_I8, 0, reg1, reg2, 0x8000));
    EXPECT_ICE("Operand constraint violated:"
               " other <= 0x7FFF || other >= UINT64_C(-0x8000)");
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_STORE_I8, 0, reg1, reg2, -0x8001));
    EXPECT_ICE("Operand constraint violated:"
               " other <= 0x7FFF || other >= UINT64_C(-0x8000)");
    EXPECT_EQ(unit->num_insns, 5);
#endif

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r1, 0xA\n"
        "    1: LOAD_IMM   r2, 20\n"
        "    2: STORE_I8   0(r1), r2\n"
        "           r1: 0xA\n"
        "           r2: 20\n"
        "    3: STORE_I8   32767(r1), r2\n"
        "           r1: 0xA\n"
        "           r2: 20\n"
        "    4: STORE_I8   -32768(r1), r2\n"
        "           r1: 0xA\n"
        "           r2: 20\n"
        "\n"
        "Block    0: <none> --> [0,4] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
