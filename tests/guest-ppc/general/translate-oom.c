/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    uint32_t illegal_insn = 0;

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.memory_base = &illegal_insn;
    setup.state_offset_gpr = 0x100;
    setup.state_offset_fpr = 0x180;
    setup.state_offset_gqr = 0x380;
    setup.state_offset_cr = 0x3A0;
    setup.state_offset_lr = 0x3A4;
    setup.state_offset_ctr = 0x3A8;
    setup.state_offset_xer = 0x3AC;
    setup.state_offset_fpscr = 0x3B0;
    setup.state_offset_reserve_flag = 0x3B4;
    setup.state_offset_reserve_address = 0x3B8;
    setup.state_offset_nia = 0x3BC;
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    binrec_set_code_range(handle, 0, 3);

    RTLUnit *unit;
    for (int count = 0; ; count++) {
        if (count >= 100) {
            FAIL("Failed to translate code after 100 tries");
        }
        EXPECT(unit = rtl_create_unit(handle));
        /* Adjust array sizes to force reallocation. */
        unit->insns_size = 1;
        unit->blocks_size = 1;
        unit->labels_size = 1;
        unit->regs_size = 1;
        unit->aliases_size = 1;
        mem_wrap_fail_after(count);
        if (guest_ppc_translate(handle, 0, unit)) {
            if (count == 0) {
                FAIL("Translation did not fail on memory allocation failure");
            }
            break;
        }
        rtl_destroy_unit(unit);
    }

    mem_wrap_cancel_fail();
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LABEL      L1\n"
                 "    2: ILLEGAL\n"
                 "    3: LOAD_IMM   r2, 4\n"
                 "    4: SET_ALIAS  a1, r2\n"
                 "    5: LABEL      L2\n"
                 "    6: GET_ALIAS  r3, a1\n"
                 "    7: STORE_I32  956(r1), r3\n"
                 "    8: RETURN     r1\n"
                 "\n"
                 "Block    0: <none> --> [0,0] --> 1\n"
                 "Block    1: 0 --> [1,4] --> 2\n"
                 "Block    2: 1 --> [5,8] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}