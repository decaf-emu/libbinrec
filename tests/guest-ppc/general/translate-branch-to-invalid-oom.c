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
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    static uint8_t input[] = {
        0x48,0x00,0x00,0x0C,  // b 0xC
        0x00,0x00,0x00,0x00,  // (invalid)
        0x00,0x00,0x00,0x00,  // (invalid)
        0x4B,0xFF,0xFF,0xFC,  // b 0x8
    };

    binrec_setup_t final_setup;
    memcpy(&final_setup, &setup, sizeof(setup));
    final_setup.guest_memory_base = input;
    final_setup.malloc = mem_wrap_malloc;
    final_setup.realloc = mem_wrap_realloc;
    final_setup.free = mem_wrap_free;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    /* Adjust instruction array size to force out-of-memory while handling
     * the empty block. */
    unit->insns_size = 8;
    mem_wrap_fail_after(1);
    EXPECT_FALSE(guest_ppc_translate(handle, 0, sizeof(input)-1, unit));
    mem_wrap_cancel_fail();
    EXPECT_ICE("Failed to translate empty block at 0x8");

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
