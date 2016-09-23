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

    int label, reg1, reg2;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    ASSERT(reg2 != label);

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg2, 0, label));
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_GOTO_IF_Z);
    EXPECT_EQ(unit->insns[2].src1, reg2);
    EXPECT_EQ(unit->insns[2].label, label);
    EXPECT(unit->have_block);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LABEL      L1\n"
        "    1: LOAD_IMM   r2, 10\n"
        "    2: GOTO_IF_Z  r2, L1\n"
        "           r2: 10\n"
        "\n"
        "Block    0: 0 --> [0,2] --> 1,0\n"
        "Block    1: 0 --> [empty] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
