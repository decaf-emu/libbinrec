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

    int reg1, reg2, reg3, reg4, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 30));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg4, reg3, 0, 40));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 3);
    EXPECT_EQ(unit->regs[reg2].birth, 3);
    EXPECT_EQ(unit->regs[reg2].death, 3);
    EXPECT_EQ(unit->regs[reg3].birth, 1);
    EXPECT_EQ(unit->regs[reg3].death, 4);
    EXPECT_EQ(unit->regs[reg4].birth, 4);
    EXPECT_EQ(unit->regs[reg4].death, 4);
    EXPECT(rtl_finalize_unit(unit));

    rtl_update_live_ranges(unit);
    /* reg1's live range is extended to the backward branch. */
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 5);
    /* reg2 is not live on entering the second block, so its live range
     * is not extended. */
    EXPECT_EQ(unit->regs[reg2].birth, 3);
    EXPECT_EQ(unit->regs[reg2].death, 3);
    /* reg3's live range is extended to the backward branch.  The
     * intervening reg2 should not cause this register to be skipped. */
    EXPECT_EQ(unit->regs[reg3].birth, 1);
    EXPECT_EQ(unit->regs[reg3].death, 5);
    /* reg4 is not live on entering the second block. */
    EXPECT_EQ(unit->regs[reg4].birth, 4);
    EXPECT_EQ(unit->regs[reg4].death, 4);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
