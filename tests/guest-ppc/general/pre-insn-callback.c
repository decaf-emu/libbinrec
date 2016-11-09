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
                 "    2: GET_ALIAS  r3, a3\n"
                 "    3: BFEXT      r4, r3, 3, 1\n"
                 "    4: SET_ALIAS  a4, r4\n"
                 "    5: BFEXT      r5, r3, 2, 1\n"
                 "    6: SET_ALIAS  a5, r5\n"
                 "    7: BFEXT      r6, r3, 1, 1\n"
                 "    8: SET_ALIAS  a6, r6\n"
                 "    9: BFEXT      r7, r3, 0, 1\n"
                 "   10: SET_ALIAS  a7, r7\n"
                 "   11: GET_ALIAS  r8, a3\n"
                 "   12: ANDI       r9, r8, -16\n"
                 "   13: GET_ALIAS  r10, a4\n"
                 "   14: SLLI       r11, r10, 3\n"
                 "   15: OR         r12, r9, r11\n"
                 "   16: GET_ALIAS  r13, a5\n"
                 "   17: SLLI       r14, r13, 2\n"
                 "   18: OR         r15, r12, r14\n"
                 "   19: GET_ALIAS  r16, a6\n"
                 "   20: SLLI       r17, r16, 1\n"
                 "   21: OR         r18, r15, r17\n"
                 "   22: GET_ALIAS  r19, a7\n"
                 "   23: OR         r20, r18, r19\n"
                 "   24: SET_ALIAS  a3, r20\n"
                 "   25: LOAD_IMM   r21, 0x1\n"
                 "   26: LOAD_IMM   r22, 4096\n"
                 "   27: CALL_TRANSPARENT @r21, r1, r22\n"
                 "   28: LOAD_IMM   r23, 1\n"
                 "   29: SET_ALIAS  a2, r23\n"
                 "   30: GET_ALIAS  r24, a3\n"
                 "   31: ANDI       r25, r24, -16\n"
                 "   32: SLLI       r26, r10, 3\n"
                 "   33: OR         r27, r25, r26\n"
                 "   34: SLLI       r28, r13, 2\n"
                 "   35: OR         r29, r27, r28\n"
                 "   36: SLLI       r30, r16, 1\n"
                 "   37: OR         r31, r29, r30\n"
                 "   38: OR         r32, r31, r19\n"
                 "   39: SET_ALIAS  a3, r32\n"
                 "   40: LOAD_IMM   r33, 0x1\n"
                 "   41: LOAD_IMM   r34, 4100\n"
                 "   42: CALL_TRANSPARENT @r33, r1, r34\n"
                 "   43: SLTSI      r35, r23, 1\n"
                 "   44: SGTSI      r36, r23, 1\n"
                 "   45: SEQI       r37, r23, 1\n"
                 "   46: GET_ALIAS  r38, a8\n"
                 "   47: BFEXT      r39, r38, 31, 1\n"
                 "   48: SET_ALIAS  a4, r35\n"
                 "   49: SET_ALIAS  a5, r36\n"
                 "   50: SET_ALIAS  a6, r37\n"
                 "   51: SET_ALIAS  a7, r39\n"
                 "   52: LOAD_IMM   r40, 4104\n"
                 "   53: SET_ALIAS  a1, r40\n"
                 "   54: GET_ALIAS  r41, a3\n"
                 "   55: ANDI       r42, r41, -16\n"
                 "   56: GET_ALIAS  r43, a4\n"
                 "   57: SLLI       r44, r43, 3\n"
                 "   58: OR         r45, r42, r44\n"
                 "   59: GET_ALIAS  r46, a5\n"
                 "   60: SLLI       r47, r46, 2\n"
                 "   61: OR         r48, r45, r47\n"
                 "   62: GET_ALIAS  r49, a6\n"
                 "   63: SLLI       r50, r49, 1\n"
                 "   64: OR         r51, r48, r50\n"
                 "   65: GET_ALIAS  r52, a7\n"
                 "   66: OR         r53, r51, r52\n"
                 "   67: SET_ALIAS  a3, r53\n"
                 "   68: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32 @ 928(r1)\n"
                 "Alias 4: int32, no bound storage\n"
                 "Alias 5: int32, no bound storage\n"
                 "Alias 6: int32, no bound storage\n"
                 "Alias 7: int32, no bound storage\n"
                 "Alias 8: int32 @ 940(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,68] --> <none>\n"
                 );

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
