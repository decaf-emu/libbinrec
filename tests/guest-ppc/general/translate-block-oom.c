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
    static uint8_t ppc_code[] = {
        0x38,0x60,0x00,0x01,  // li r3,1
        0x48,0x00,0x00,0x08,  // b 0xC
        0x38,0x60,0x00,0x02,  // li r3,2
        0x60,0x00,0x00,0x00,  // nop
    };

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest_memory_base = ppc_code;
    setup.state_offset_gpr = 0x100;
    setup.state_offset_fpr = 0x180;
    setup.state_offset_gqr = 0x380;
    setup.state_offset_cr = 0x3A0;
    setup.state_offset_lr = 0x3A4;
    setup.state_offset_ctr = 0x3A8;
    setup.state_offset_xer = 0x3AC;
    setup.state_offset_fpscr = 0x3B0;
    setup.state_offset_reserve_flag = 0x3B4;
    setup.state_offset_reserve_state = 0x3B8;
    setup.state_offset_nia = 0x3BC;
    setup.state_offset_timebase_handler = 0x3C8;
    setup.state_offset_sc_handler = 0x3D0;
    setup.state_offset_trap_handler = 0x3D8;
    setup.state_offset_chain_lookup = 0x3E0;
    setup.state_offset_branch_exit_flag = 0x3E8;
    setup.state_offset_fres_lut = 0x3F0;
    setup.state_offset_frsqrte_lut = 0x3F8;
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    for (int count = 1; ; count++) {
        if (count >= 100) {
            FAIL("Failed to translate code after 100 tries");
        }
        EXPECT(unit = rtl_create_unit(handle));
        /* Adjust instruction array size to force reallocation (which will
         * fail). */
        unit->insns_size = count;
        mem_wrap_fail_after(1);
        if (guest_ppc_translate(handle, 0, sizeof(ppc_code)-1, unit)) {
            if (count == 1) {
                FAIL("Translation did not fail on memory allocation failure");
            }
            break;
        }
        mem_wrap_cancel_fail();
        rtl_destroy_unit(unit);
    }

    mem_wrap_cancel_fail();
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LOAD_ARG   r2, 1\n"
                 "    2: LOAD_IMM   r3, 1\n"
                 "    3: SET_ALIAS  a2, r3\n"
                 "    4: GOTO       L1\n"
                 "    5: LOAD_IMM   r4, 8\n"
                 "    6: SET_ALIAS  a1, r4\n"
                 "    7: LOAD_IMM   r5, 2\n"
                 "    8: SET_ALIAS  a2, r5\n"
                 "    9: LOAD_IMM   r6, 12\n"
                 "   10: SET_ALIAS  a1, r6\n"
                 "   11: LABEL      L1\n"
                 "   12: LOAD_IMM   r7, 16\n"
                 "   13: SET_ALIAS  a1, r7\n"
                 "   14: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "\n"
                 /* We don't call rtl_finalize_unit(), so this edge is
                  * missing. */
                 "Block 0: <none> --> [0,4] --> <none>\n"
                 "Block 1: <none> --> [5,10] --> 2\n"
                 "Block 2: 1 --> [11,14] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
