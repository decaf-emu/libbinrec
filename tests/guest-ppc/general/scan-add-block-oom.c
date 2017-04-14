/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/guest-ppc.h"
#include "src/guest-ppc/guest-ppc-internal.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/guest-ppc/insn/common.h"
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    static uint8_t input[] = {
        0x48,0x00,0x00,0x04,  // b 0x4
        0x60,0x00,0x00,0x00,  // nop
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

    GuestPPCContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.handle = handle;
    ctx.unit = unit;
    ctx.start = 0;
    EXPECT(ctx.blocks = rtl_malloc(unit, sizeof(*ctx.blocks)));
    ctx.blocks_size = 1;
    memset(ctx.blocks, 0, sizeof(*ctx.blocks));

    mem_wrap_fail_after(0);
    EXPECT_FALSE(guest_ppc_scan(&ctx, 7));
    mem_wrap_cancel_fail();

    EXPECT_STREQ(get_log_messages(), "[error] Out of memory expanding basic"
                 " block table for branch target 0x4 at 0x0\n");

    rtl_free(unit, ctx.blocks);
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
