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

    int reg1, reg2, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT_EQ(unit->num_blocks, 2);
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    EXPECT_FALSE(unit->have_block);
    EXPECT_EQ(unit->num_blocks, 3);
    EXPECT_EQ(unit->num_insns, 5);
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 2);
    EXPECT_EQ(unit->regs[reg2].birth, 2);
    EXPECT_EQ(unit->regs[reg2].death, 2);

    /* Add an empty block with an edge to block 1.  This should not
     * affect extension of live ranges even though its nominal first
     * instruction index is the latest of all block 1's predecessors. */
    EXPECT(rtl_block_add(unit));
    EXPECT(rtl_block_add_edge(unit, 3, 1));

    EXPECT(rtl_finalize_unit(unit));
    /* reg1's live range is extended to the backward branch. */
    EXPECT_EQ(unit->regs[reg1].birth, 0);
    EXPECT_EQ(unit->regs[reg1].death, 3);
    /* reg2 is not live on entering the second block, so its live range
     * is not extended. */
    EXPECT_EQ(unit->regs[reg2].birth, 2);
    EXPECT_EQ(unit->regs[reg2].death, 2);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
