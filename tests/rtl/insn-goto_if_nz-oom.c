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

    uint32_t label, reg1, reg2;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    ASSERT(reg2 != label);

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 10));
    EXPECT_EQ(unit->num_insns, 2);
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT(unit->have_block);
    EXPECT_EQ(unit->cur_block, 0);

    unit->blocks_size = 1;
    mem_wrap_fail_after(0);
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg2, 0, label));
    EXPECT_ICE("Failed to start a new basic block at 2");
    EXPECT_EQ(unit->num_insns, 2);
    EXPECT_EQ(unit->num_blocks, 1);
    EXPECT(unit->have_block);
    EXPECT_EQ(unit->cur_block, 0);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
