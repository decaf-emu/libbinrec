/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/endian.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/guest-ppc/common.h"


int main(void)
{
    uint8_t *memory;
    EXPECT(memory = malloc(0x10000));

    static const uint32_t ppc_code[] = {
        0x38600001,  // li r3,1
        0x3880000A,  // li r4,10
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest_memory_base = memory;
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
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    binrec_set_pre_insn_callback(handle, (void (*)(void *, uint32_t))1);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT(guest_ppc_translate(handle, 0x1000, 0x1007, unit));
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LOAD_IMM   r2, 0x100000000\n"
                 "    2: LABEL      L1\n"
                 "    3: LOAD_IMM   r3, 0x1\n"
                 "    4: LOAD_IMM   r4, 4096\n"
                 "    5: CALL_TRANSPARENT @r3, r1, r4\n"
                 "    6: LOAD_IMM   r5, 1\n"
                 "    7: SET_ALIAS  a2, r5\n"
                 "    8: LOAD_IMM   r6, 0x1\n"
                 "    9: LOAD_IMM   r7, 4100\n"
                 "   10: CALL_TRANSPARENT @r6, r1, r7\n"
                 "   11: LOAD_IMM   r8, 10\n"
                 "   12: SET_ALIAS  a2, r5\n"
                 "   13: SET_ALIAS  a3, r8\n"
                 "   14: LOAD_IMM   r9, 4104\n"
                 "   15: SET_ALIAS  a1, r9\n"
                 "   16: LABEL      L2\n"
                 "   17: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32 @ 272(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,1] --> 1\n"
                 "Block 1: 0 --> [2,15] --> 2\n"
                 "Block 2: 1 --> [16,17] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
