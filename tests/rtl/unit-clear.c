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

    EXPECT(rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    EXPECT_FALSE(rtl_add_insn(unit, RTLOP_LOAD_IMM, 0, 0, 0, 0));
    EXPECT(rtl_finalize_unit(unit));

    clear_log_messages();
    rtl_clear_unit(unit);

    EXPECT_PTREQ(unit->handle, handle);
    EXPECT(unit->blocks);
    EXPECT(unit->blocks_size > 0);
    EXPECT_EQ(unit->num_blocks, 0);
    EXPECT_FALSE(unit->have_block);
    EXPECT(unit->label_blockmap);
    EXPECT_EQ(unit->next_label, 1);
    EXPECT(unit->regs);
    EXPECT_EQ(unit->next_reg, 1);
    EXPECT(unit->aliases);
    EXPECT_EQ(unit->next_alias, 1);
    EXPECT_FALSE(unit->error);
    EXPECT_FALSE(unit->finalized);

    EXPECT_STREQ(get_log_messages(), NULL);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
