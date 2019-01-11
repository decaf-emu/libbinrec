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
    uint32_t illegal_insn = 0;

    binrec_setup_t final_setup;
    memcpy(&final_setup, &setup, sizeof(setup));
    final_setup.guest_memory_base = &illegal_insn;
    final_setup.malloc = mem_wrap_malloc;
    final_setup.realloc = mem_wrap_realloc;
    final_setup.free = mem_wrap_free;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

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
        if (guest_ppc_translate(handle, 0, 3, unit)) {
            if (count == 0) {
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
                 "    2: ILLEGAL\n"
                 "    3: LOAD_IMM   r3, 4\n"
                 "    4: SET_ALIAS  a1, r3\n"
                 "    5: RETURN     r1\n"
                 "\n"
                 "Alias 1: int32 @ 964(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,5] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
