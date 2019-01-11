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
#include "tests/guest-ppc/insn/common.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    static uint8_t input[] = {
        0x38,0x60,0x00,0x01,  // li r3,1
        0x48,0x00,0x00,0x08,  // b 0xC
        0x38,0x60,0x00,0x02,  // li r3,2
        0x60,0x00,0x00,0x00,  // nop
    };

    binrec_setup_t final_setup;
    memcpy(&final_setup, &setup, sizeof(setup));
    final_setup.guest_memory_base = input;
    final_setup.malloc = mem_wrap_malloc;
    final_setup.realloc = mem_wrap_realloc;
    final_setup.free = mem_wrap_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

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
        if (guest_ppc_translate(handle, 0, sizeof(input)-1, unit)) {
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
                 "   14: RETURN     r1\n"
                 "\n"
                 "Alias 1: int32 @ 964(r1)\n"
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
