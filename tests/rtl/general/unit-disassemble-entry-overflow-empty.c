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

    int reg, label;
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(label = rtl_alloc_label(unit));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_NZ, 0, reg, 0, label));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    EXPECT(rtl_finalize_unit(unit));

    /* Remove the solitary entry edge in block 0 (as opposed to the ones
     * in the extension block) so the base entry list is empty.  This
     * should not cause the code to think that the block has no entering
     * edges at all. */
    rtl_block_remove_edge(unit, 0, 1);
    EXPECT_EQ(unit->blocks[0].entries[0], -1);
    EXPECT_EQ(unit->blocks[0].entry_overflow, 8);
    EXPECT_EQ(unit->blocks[0].exits[0], 1);
    EXPECT_EQ(unit->blocks[0].exits[1], -1);

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, false));
    EXPECT_STREQ(disassembly,
                 "    0: LABEL      L1\n"
                 "    1: LOAD_ARG   r1, 0\n"
                 "    2: GOTO_IF_NZ r1, L1\n"
                 "    3: GOTO_IF_NZ r1, L1\n"
                 "    4: GOTO_IF_NZ r1, L1\n"
                 "    5: GOTO_IF_NZ r1, L1\n"
                 "    6: GOTO_IF_NZ r1, L1\n"
                 "    7: GOTO_IF_NZ r1, L1\n"
                 "    8: GOTO_IF_NZ r1, L1\n"
                 "    9: GOTO       L1\n"
                 "\n"
                 "Block 0: 7,1,2,3,4,5,6 --> [0,2] --> 1\n"
                 "Block 1: 0 --> [3,3] --> 2,0\n"
                 "Block 2: 1 --> [4,4] --> 3,0\n"
                 "Block 3: 2 --> [5,5] --> 4,0\n"
                 "Block 4: 3 --> [6,6] --> 5,0\n"
                 "Block 5: 4 --> [7,7] --> 6,0\n"
                 "Block 6: 5 --> [8,8] --> 7,0\n"
                 "Block 7: 6 --> [9,9] --> 0\n");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
