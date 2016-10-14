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
        0x2F830001,  // cmpi cr7,r3,1
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
    setup.state_offset_branch_callback = 0x3E0;
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
                 "   11: SLTSI      r8, r5, 1\n"
                 "   12: SGTSI      r9, r5, 1\n"
                 "   13: SEQI       r10, r5, 1\n"
                 "   14: GET_ALIAS  r11, a8\n"
                 "   15: BFEXT      r12, r11, 31, 1\n"
                 "   16: SET_ALIAS  a2, r5\n"
                 "   17: SET_ALIAS  a3, r8\n"
                 "   18: SET_ALIAS  a4, r9\n"
                 "   19: SET_ALIAS  a5, r10\n"
                 "   20: SET_ALIAS  a6, r12\n"
                 "   21: LOAD_IMM   r13, 4104\n"
                 "   22: SET_ALIAS  a1, r13\n"
                 "   23: LABEL      L2\n"
                 "   24: GET_ALIAS  r14, a7\n"
                 "   25: ANDI       r15, r14, -16\n"
                 "   26: GET_ALIAS  r16, a3\n"
                 "   27: SLLI       r17, r16, 3\n"
                 "   28: OR         r18, r15, r17\n"
                 "   29: GET_ALIAS  r19, a4\n"
                 "   30: SLLI       r20, r19, 2\n"
                 "   31: OR         r21, r18, r20\n"
                 "   32: GET_ALIAS  r22, a5\n"
                 "   33: SLLI       r23, r22, 1\n"
                 "   34: OR         r24, r21, r23\n"
                 "   35: GET_ALIAS  r25, a6\n"
                 "   36: OR         r26, r24, r25\n"
                 "   37: SET_ALIAS  a7, r26\n"
                 "   38: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32, no bound storage\n"
                 "Alias 4: int32, no bound storage\n"
                 "Alias 5: int32, no bound storage\n"
                 "Alias 6: int32, no bound storage\n"
                 "Alias 7: int32 @ 928(r1)\n"
                 "Alias 8: int32 @ 940(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,1] --> 1\n"
                 "Block 1: 0 --> [2,22] --> 2\n"
                 "Block 2: 1 --> [23,38] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
