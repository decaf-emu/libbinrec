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
    setup.state_offset_fres_lut = 0x3E8;
    setup.state_offset_frsqrte_lut = 0x3F0;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    binrec_set_post_insn_callback(handle, (void (*)(void *, uint32_t))2);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT(guest_ppc_translate(handle, 0x1000, 0x1007, unit));
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LOAD_ARG   r2, 1\n"
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
                 "   12: GET_ALIAS  r10, a4\n"
                 "   13: BFEXT      r11, r10, 31, 1\n"
                 "   14: GET_ALIAS  r12, a3\n"
                 "   15: SLLI       r13, r7, 3\n"
                 "   16: SLLI       r14, r8, 2\n"
                 "   17: SLLI       r15, r9, 1\n"
                 "   18: OR         r16, r13, r14\n"
                 "   19: OR         r17, r15, r11\n"
                 "   20: OR         r18, r16, r17\n"
                 "   21: BFINS      r19, r12, r18, 0, 4\n"
                 "   22: SET_ALIAS  a3, r19\n"
                 "   23: LOAD_IMM   r20, 4104\n"
                 "   24: SET_ALIAS  a1, r20\n"
                 "   25: LOAD_IMM   r21, 0x2\n"
                 "   26: LOAD_IMM   r22, 4100\n"
                 "   27: CALL_TRANSPARENT @r21, r1, r22\n"
                 "   28: LOAD_IMM   r23, 4104\n"
                 "   29: SET_ALIAS  a1, r23\n"
                 "   30: RETURN\n"
                 "\n"
                 "Alias 1: int32 @ 956(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32 @ 928(r1)\n"
                 "Alias 4: int32 @ 940(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,30] --> <none>\n"
                 );

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
