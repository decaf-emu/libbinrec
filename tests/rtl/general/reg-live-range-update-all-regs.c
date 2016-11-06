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

    int reg1, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 2);
    EXPECT(rtl_finalize_unit(unit));

    EXPECT(unit->next_reg == reg1 + 1);
    EXPECT(unit->regs_size > reg1 + 1);
    unit->regs[reg1 + 1] = unit->regs[reg1];

    rtl_update_live_ranges(unit);
    /* reg1's live range is extended to the backward branch. */
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 3);
    /* The next entry in unit->regs[] should not be touched since it hasn't
     * been allocated. */
    EXPECT_EQ(unit->regs[reg1 + 1].birth, 0);
    EXPECT_EQ(unit->regs[reg1 + 1].death, 2);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
