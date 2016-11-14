/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/common.h"
#include "src/endian.h"
#include "src/rtl-internal.h"  // For BLOCKS_EXPAND_SIZE.
#include "tests/common.h"
#include "tests/execute.h"
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    uint32_t input[BLOCKS_EXPAND_SIZE - 3];
    const int nop_index = lenof(input) - 1;
    for (int i = 0; i < nop_index; i++) {
        input[i] = bswap_be32(0x48000000 + 4 * (nop_index - i));  // b end
    }
    input[nop_index] = bswap_be32(0x60000000);  // end: nop

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = BINREC_ARCH_PPC_7XX;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.guest_memory_base = input;
    setup.state_offset_gpr = offsetof(PPCState,gpr);
    setup.state_offset_fpr = offsetof(PPCState,fpr);
    setup.state_offset_gqr = offsetof(PPCState,gqr);
    setup.state_offset_cr = offsetof(PPCState,cr);
    setup.state_offset_lr = offsetof(PPCState,lr);
    setup.state_offset_ctr = offsetof(PPCState,ctr);
    setup.state_offset_xer = offsetof(PPCState,xer);
    setup.state_offset_fpscr = offsetof(PPCState,fpscr);
    setup.state_offset_reserve_flag = offsetof(PPCState,reserve_flag);
    setup.state_offset_reserve_state = offsetof(PPCState,reserve_state);
    setup.state_offset_nia = offsetof(PPCState,nia);
    setup.state_offset_timebase_handler = offsetof(PPCState,timebase_handler);
    setup.state_offset_sc_handler = offsetof(PPCState,sc_handler);
    setup.state_offset_trap_handler = offsetof(PPCState,trap_handler);
    setup.state_offset_fres_lut = offsetof(PPCState,fres_lut);
    setup.state_offset_frsqrte_lut = offsetof(PPCState,frsqrte_lut);
    setup.malloc = mem_wrap_malloc;
    setup.realloc = mem_wrap_realloc;
    setup.free = mem_wrap_free;
    setup.log = log_capture;

    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    mem_wrap_fail_after(10);
    EXPECT_FALSE(binrec_translate(handle, 0, sizeof(input) - 1,
                                  (void *[1]){}, (long[1]){}));
    mem_wrap_cancel_fail();

    const char *log_messages = get_log_messages();
    EXPECT(log_messages);
    const char *last_line = log_messages;
    for (;;) {
        const char *eol;
        EXPECT(eol = strchr(last_line, '\n'));
        if (!eol[1]) {
            break;
        }
        last_line = eol + 1;
    }
    if (strcmp(last_line,
               "[error] Failed to finalize RTL for code at 0x0\n") != 0) {
        fputs(log_messages, stdout);
        FAIL("binrec_translate(handle, 0x0, 0x%X, ...) did not return the"
             " expected error", (int)sizeof(input) - 1);
    }

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
