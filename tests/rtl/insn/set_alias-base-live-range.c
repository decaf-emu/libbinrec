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

    int reg1, reg2, reg3, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    ASSERT(reg2 != alias);
    ASSERT(reg3 != alias);
    rtl_set_alias_storage(unit, alias, reg3, 16);

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    EXPECT_EQ(unit->num_insns, 3);
    EXPECT_EQ(unit->insns[2].opcode, RTLOP_SET_ALIAS);
    EXPECT_EQ(unit->insns[2].src1, reg2);
    EXPECT_EQ(unit->insns[2].alias, alias);
    EXPECT_EQ(unit->regs[reg2].birth, 1);
    EXPECT_EQ(unit->regs[reg2].death, 2);
    EXPECT_EQ(unit->regs[reg3].birth, 0);
    EXPECT_EQ(unit->regs[reg3].death, 2);
    EXPECT(unit->have_block);
    EXPECT_FALSE(unit->error);

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly =
        "    0: LOAD_IMM   r3, 0xA\n"
        "    1: LOAD_IMM   r2, 20\n"
        "    2: SET_ALIAS  a1, r2\n"
        "           r2: 20\n"
        "\n"
        "Alias 1: int32 @ 16(r3)\n"
        "\n"
        "Block 0: <none> --> [0,2] --> <none>\n"
        ;
    EXPECT_STREQ(rtl_disassemble_unit(unit, true), disassembly);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
