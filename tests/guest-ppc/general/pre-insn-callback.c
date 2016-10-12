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

    binrec_set_pre_insn_callback(handle, (void (*)(void *, uint32_t))1);

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
                 "    6: LOAD       r5, 928(r1)\n"
                 "    7: ANDI       r6, r5, -16\n"
                 "    8: GET_ALIAS  r7, a3\n"
                 "    9: OR         r8, r6, r7\n"
                 "   10: STORE      928(r1), r8\n"
                 "   11: LOAD_IMM   r9, 0x1\n"
                 "   12: LOAD_IMM   r10, 4096\n"
                 "   13: CALL_TRANSPARENT @r9, r1, r10\n"
                 "   14: LOAD_IMM   r11, 1\n"
                 "   15: SET_ALIAS  a2, r11\n"
                 "   16: LOAD       r12, 928(r1)\n"
                 "   17: ANDI       r13, r12, -16\n"
                 "   18: OR         r14, r13, r7\n"
                 "   19: STORE      928(r1), r14\n"
                 "   20: LOAD_IMM   r15, 0x1\n"
                 "   21: LOAD_IMM   r16, 4100\n"
                 "   22: CALL_TRANSPARENT @r15, r1, r16\n"
                 "   23: SLTSI      r17, r11, 1\n"
                 "   24: SGTSI      r18, r11, 1\n"
                 "   25: SEQI       r19, r11, 1\n"
                 "   26: GET_ALIAS  r21, a4\n"
                 "   27: BFEXT      r20, r21, 31, 1\n"
                 "   28: SLLI       r22, r19, 1\n"
                 "   29: OR         r23, r20, r22\n"
                 "   30: SLLI       r24, r18, 2\n"
                 "   31: OR         r25, r23, r24\n"
                 "   32: SLLI       r26, r17, 3\n"
                 "   33: OR         r27, r25, r26\n"
                 "   34: SET_ALIAS  a2, r11\n"
                 "   35: SET_ALIAS  a3, r27\n"
                 "   36: LOAD_IMM   r28, 4104\n"
                 "   37: SET_ALIAS  a1, r28\n"
                 "   38: LABEL      L2\n"
                 "   39: LOAD       r29, 928(r1)\n"
                 "   40: ANDI       r30, r29, -16\n"
                 "   41: GET_ALIAS  r31, a3\n"
                 "   42: OR         r32, r30, r31\n"
                 "   43: STORE      928(r1), r32\n"
                 "   44: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32, no bound storage\n"
                 "Alias 4: int32 @ 940(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,4] --> 1\n"
                 "Block 1: 0 --> [5,37] --> 2\n"
                 "Block 2: 1 --> [38,44] --> <none>\n"
                 );


    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
