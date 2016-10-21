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

    binrec_set_post_insn_callback(handle, (void (*)(void *, uint32_t))2);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT(guest_ppc_translate(handle, 0x1000, 0x1007, unit));
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LOAD_IMM   r2, 0x100000000\n"
                 "    2: LOAD_IMM   r3, 1\n"
                 "    3: SET_ALIAS  a2, r3\n"
                 "    4: LOAD_IMM   r4, 4100\n"
                 "    5: SET_ALIAS  a1, r4\n"
                 "    6: LOAD_IMM   r5, 0x2\n"
                 "    7: LOAD_IMM   r6, 4096\n"
                 "    8: CALL_TRANSPARENT @r5, r1, r6\n"
                 "    9: SLTSI      r7, r3, 1\n"
                 "   10: SGTSI      r8, r3, 1\n"
                 "   11: SEQI       r9, r3, 1\n"
                 "   12: GET_ALIAS  r10, a8\n"
                 "   13: BFEXT      r11, r10, 31, 1\n"
                 "   14: SET_ALIAS  a3, r7\n"
                 "   15: SET_ALIAS  a4, r8\n"
                 "   16: SET_ALIAS  a5, r9\n"
                 "   17: SET_ALIAS  a6, r11\n"
                 "   18: LOAD_IMM   r12, 4104\n"
                 "   19: SET_ALIAS  a1, r12\n"
                 "   20: GET_ALIAS  r13, a7\n"
                 "   21: ANDI       r14, r13, -16\n"
                 "   22: SLLI       r15, r7, 3\n"
                 "   23: OR         r16, r14, r15\n"
                 "   24: SLLI       r17, r8, 2\n"
                 "   25: OR         r18, r16, r17\n"
                 "   26: SLLI       r19, r9, 1\n"
                 "   27: OR         r20, r18, r19\n"
                 "   28: OR         r21, r20, r11\n"
                 "   29: SET_ALIAS  a7, r21\n"
                 "   30: LOAD_IMM   r22, 0x2\n"
                 "   31: LOAD_IMM   r23, 4100\n"
                 "   32: CALL_TRANSPARENT @r22, r1, r23\n"
                 "   33: LOAD_IMM   r24, 4104\n"
                 "   34: SET_ALIAS  a1, r24\n"
                 "   35: GET_ALIAS  r25, a7\n"
                 "   36: ANDI       r26, r25, -16\n"
                 "   37: GET_ALIAS  r27, a3\n"
                 "   38: SLLI       r28, r27, 3\n"
                 "   39: OR         r29, r26, r28\n"
                 "   40: GET_ALIAS  r30, a4\n"
                 "   41: SLLI       r31, r30, 2\n"
                 "   42: OR         r32, r29, r31\n"
                 "   43: GET_ALIAS  r33, a5\n"
                 "   44: SLLI       r34, r33, 1\n"
                 "   45: OR         r35, r32, r34\n"
                 "   46: GET_ALIAS  r36, a6\n"
                 "   47: OR         r37, r35, r36\n"
                 "   48: SET_ALIAS  a7, r37\n"
                 "   49: RETURN\n"
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
                 "Block 0: <none> --> [0,49] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
