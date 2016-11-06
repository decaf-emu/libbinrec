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

    RTLBlock *block;
    const int num_entries = lenof(block->entries);

    for (int i = 0; i < num_entries; i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    }

    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    const int block_index = unit->cur_block;
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 20));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT_EQ(unit->regs[reg1].birth, num_entries);
    EXPECT_EQ(unit->regs[reg1].death, num_entries + 2);
    EXPECT_EQ(unit->regs[reg2].birth, num_entries + 2);
    EXPECT_EQ(unit->regs[reg2].death, num_entries + 2);
    EXPECT(rtl_finalize_unit(unit));

    block = &unit->blocks[block_index];
    EXPECT(block->entry_overflow >= 0);
    /* The overflow block should contain exactly the set of edges we added
     * in the GOTO-adding loop above.  These all precede the block in
     * question, so if the code incorrectly takes the last edge found as
     * the latest predecessor block, live range updates will fail. */
    RTLBlock *overflow_block = &unit->blocks[block->entry_overflow];
    EXPECT(overflow_block->entries[num_entries - 1] < block_index);

    rtl_update_live_ranges(unit);
    /* reg1's live range is extended to the backward branch. */
    EXPECT_EQ(unit->regs[reg1].birth, num_entries);
    EXPECT_EQ(unit->regs[reg1].death, num_entries + 3);
    /* reg2 is not live on entering the second block, so its live range
     * is not extended. */
    EXPECT_EQ(unit->regs[reg2].birth, num_entries + 2);
    EXPECT_EQ(unit->regs[reg2].death, num_entries + 2);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
