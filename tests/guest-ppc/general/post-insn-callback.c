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
        0x2F830001,  // cmpwi cr7,r3,1
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
                 "    2: LOAD       r3, 928(r1)\n"
                 "    3: BFEXT      r4, r3, 0, 4\n"
                 "    4: SET_ALIAS  a3, r4\n"
                 "    5: LABEL      L1\n"
                 "    6: LOAD_IMM   r5, 1\n"
                 "    7: LOAD_IMM   r6, 4100\n"
                 "    8: SET_ALIAS  a1, r6\n"
                 "    9: SET_ALIAS  a2, r5\n"
                 "   10: LOAD       r7, 928(r1)\n"
                 "   11: ANDI       r8, r7, -16\n"
                 "   12: GET_ALIAS  r9, a3\n"
                 "   13: OR         r10, r8, r9\n"
                 "   14: STORE      928(r1), r10\n"
                 "   15: LOAD_IMM   r11, 0x2\n"
                 "   16: LOAD_IMM   r12, 4096\n"
                 "   17: CALL_TRANSPARENT @r11, r1, r12\n"
                 "   18: SLTSI      r13, r5, 1\n"
                 "   19: SGTSI      r14, r5, 1\n"
                 "   20: SEQI       r15, r5, 1\n"
                 "   21: GET_ALIAS  r17, a4\n"
                 "   22: BFEXT      r16, r17, 31, 1\n"
                 "   23: SLLI       r18, r15, 1\n"
                 "   24: OR         r19, r16, r18\n"
                 "   25: SLLI       r20, r14, 2\n"
                 "   26: OR         r21, r19, r20\n"
                 "   27: SLLI       r22, r13, 3\n"
                 "   28: OR         r23, r21, r22\n"
                 "   29: LOAD_IMM   r24, 4104\n"
                 "   30: SET_ALIAS  a1, r24\n"
                 "   31: SET_ALIAS  a2, r5\n"
                 "   32: SET_ALIAS  a3, r23\n"
                 "   33: LOAD       r25, 928(r1)\n"
                 "   34: ANDI       r26, r25, -16\n"
                 "   35: OR         r27, r26, r23\n"
                 "   36: STORE      928(r1), r27\n"
                 "   37: LOAD_IMM   r28, 0x2\n"
                 "   38: LOAD_IMM   r29, 4100\n"
                 "   39: CALL_TRANSPARENT @r28, r1, r29\n"
                 "   40: SET_ALIAS  a2, r5\n"
                 "   41: SET_ALIAS  a3, r23\n"
                 "   42: LOAD_IMM   r30, 4104\n"
                 "   43: SET_ALIAS  a1, r30\n"
                 "   44: LABEL      L2\n"
                 "   45: LOAD       r31, 928(r1)\n"
                 "   46: ANDI       r32, r31, -16\n"
                 "   47: GET_ALIAS  r33, a3\n"
                 "   48: OR         r34, r32, r33\n"
                 "   49: STORE      928(r1), r34\n"
                 "   50: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32, no bound storage\n"
                 "Alias 4: int32 @ 940(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,4] --> 1\n"
                 "Block 1: 0 --> [5,43] --> 2\n"
                 "Block 2: 1 --> [44,50] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
