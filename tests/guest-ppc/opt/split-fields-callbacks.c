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
        0x2F830001,  // cmpi cr7,r3,1
        0x419E0004,  // beq cr7,0x1008
        0x38600001,  // li r3,1
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

    binrec_set_optimization_flags(handle,
                                  0, BINREC_OPT_G_PPC_USE_SPLIT_FIELDS, 0);

    /* None of these callbacks should cause CR or FPSCR to be merged back
     * to the state block before the call. */
    binrec_set_pre_insn_callback(handle, (void (*)(void *, uint32_t))1);
    binrec_set_post_insn_callback(handle, (void (*)(void *, uint32_t))2);
    binrec_enable_branch_callback(handle, 1);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT(guest_ppc_translate(handle, 0x1000, 0x100B, unit));
    EXPECT(rtl_finalize_unit(unit));
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LOAD_ARG   r2, 1\n"
                 "    2: GET_ALIAS  r3, a3\n"
                 "    3: BFEXT      r4, r3, 3, 1\n"
                 "    4: SET_ALIAS  a4, r4\n"
                 "    5: BFEXT      r5, r3, 2, 1\n"
                 "    6: SET_ALIAS  a5, r5\n"
                 "    7: BFEXT      r6, r3, 1, 1\n"
                 "    8: SET_ALIAS  a6, r6\n"
                 "    9: BFEXT      r7, r3, 0, 1\n"
                 "   10: SET_ALIAS  a7, r7\n"
                 "   11: LOAD_IMM   r8, 0x1\n"
                 "   12: LOAD_IMM   r9, 4096\n"
                 "   13: CALL_TRANSPARENT @r8, r1, r9\n"
                 "   14: GET_ALIAS  r10, a2\n"
                 "   15: SLTSI      r11, r10, 1\n"
                 "   16: SGTSI      r12, r10, 1\n"
                 "   17: SEQI       r13, r10, 1\n"
                 "   18: GET_ALIAS  r14, a8\n"
                 "   19: BFEXT      r15, r14, 31, 1\n"
                 "   20: SET_ALIAS  a4, r11\n"
                 "   21: SET_ALIAS  a5, r12\n"
                 "   22: SET_ALIAS  a6, r13\n"
                 "   23: SET_ALIAS  a7, r15\n"
                 "   24: LOAD_IMM   r16, 4100\n"
                 "   25: SET_ALIAS  a1, r16\n"
                 "   26: LOAD_IMM   r17, 0x2\n"
                 "   27: LOAD_IMM   r18, 4096\n"
                 "   28: CALL_TRANSPARENT @r17, r1, r18\n"
                 "   29: LOAD_IMM   r19, 0x1\n"
                 "   30: LOAD_IMM   r20, 4100\n"
                 "   31: CALL_TRANSPARENT @r19, r1, r20\n"
                 "   32: GOTO_IF_Z  r13, L2\n"
                 "   33: LOAD_IMM   r21, 4104\n"
                 "   34: SET_ALIAS  a1, r21\n"
                 "   35: LOAD_IMM   r22, 0x2\n"
                 "   36: LOAD_IMM   r23, 4100\n"
                 "   37: CALL_TRANSPARENT @r22, r1, r23\n"
                 "   38: LOAD       r24, 992(r1)\n"
                 "   39: LOAD_IMM   r25, 4100\n"
                 "   40: CALL       r26, @r24, r1, r25\n"
                 "   41: GOTO_IF_Z  r26, L3\n"
                 "   42: GOTO       L1\n"
                 "   43: LABEL      L2\n"
                 "   44: LOAD_IMM   r27, 4104\n"
                 "   45: SET_ALIAS  a1, r27\n"
                 "   46: LOAD_IMM   r28, 0x2\n"
                 "   47: LOAD_IMM   r29, 4100\n"
                 "   48: CALL_TRANSPARENT @r28, r1, r29\n"
                 "   49: LOAD_IMM   r30, 4104\n"
                 "   50: SET_ALIAS  a1, r30\n"
                 "   51: LABEL      L1\n"
                 "   52: LOAD_IMM   r31, 0x1\n"
                 "   53: LOAD_IMM   r32, 4104\n"
                 "   54: CALL_TRANSPARENT @r31, r1, r32\n"
                 "   55: LOAD_IMM   r33, 1\n"
                 "   56: SET_ALIAS  a2, r33\n"
                 "   57: LOAD_IMM   r34, 4108\n"
                 "   58: SET_ALIAS  a1, r34\n"
                 "   59: LOAD_IMM   r35, 0x2\n"
                 "   60: LOAD_IMM   r36, 4104\n"
                 "   61: CALL_TRANSPARENT @r35, r1, r36\n"
                 "   62: LOAD_IMM   r37, 4108\n"
                 "   63: SET_ALIAS  a1, r37\n"
                 "   64: LABEL      L3\n"
                 "   65: GET_ALIAS  r38, a3\n"
                 "   66: ANDI       r39, r38, -16\n"
                 "   67: GET_ALIAS  r40, a4\n"
                 "   68: SLLI       r41, r40, 3\n"
                 "   69: OR         r42, r39, r41\n"
                 "   70: GET_ALIAS  r43, a5\n"
                 "   71: SLLI       r44, r43, 2\n"
                 "   72: OR         r45, r42, r44\n"
                 "   73: GET_ALIAS  r46, a6\n"
                 "   74: SLLI       r47, r46, 1\n"
                 "   75: OR         r48, r45, r47\n"
                 "   76: GET_ALIAS  r49, a7\n"
                 "   77: OR         r50, r48, r49\n"
                 "   78: SET_ALIAS  a3, r50\n"
                 "   79: RETURN\n"
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
                 "Block 0: <none> --> [0,32] --> 1,3\n"
                 "Block 1: 0 --> [33,41] --> 2,5\n"
                 "Block 2: 1 --> [42,42] --> 4\n"
                 "Block 3: 0 --> [43,50] --> 4\n"
                 "Block 4: 3,2 --> [51,63] --> 5\n"
                 "Block 5: 4,1 --> [64,79] --> <none>\n"
                 );

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
