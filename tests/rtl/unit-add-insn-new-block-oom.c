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
#include "tests/mem-wrappers.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    int label;
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));
    EXPECT_EQ(unit->num_insns, 1);
    EXPECT_EQ(unit->insns[0].opcode, RTLOP_GOTO);
    EXPECT_EQ(unit->insns[0].label, label);
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT_FALSE(unit->have_block);

    unit->blocks_size = 1;
    mem_wrap_fail_after(0);
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT_EQ(unit->num_insns, 1);
    EXPECT_EQ(unit->insns[0].opcode, RTLOP_GOTO);
    EXPECT_EQ(unit->insns[0].label, label);
    EXPECT_EQ(unit->blocks_size, 1);
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT_FALSE(unit->have_block);

    char expected_log[100];
    snprintf(expected_log, sizeof(expected_log),
             "[error] No memory to expand unit to %u blocks\n",
             1 + BLOCKS_EXPAND_SIZE);
    EXPECT_STREQ(get_log_messages(), expected_log);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
