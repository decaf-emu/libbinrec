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
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    static uint8_t input[] = {
        0x48,0x00,0x00,0x04,  // b 0x4
        0x60,0x00,0x00,0x00,  // nop
    };

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest_memory_base = input;
    setup.host_memory_base = UINT64_C(0x100000000);
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
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

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
