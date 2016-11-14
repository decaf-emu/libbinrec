/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

static const uint8_t input[] = {
    0xFC,0x20,0x08,0x18,  // frsp f1,f1
    0x13,0x81,0x10,0x80,  // ps_cmpu1 cr7,f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a2\n"
    "   12: VEXTRACT   r9, r8, 0\n"
    "   13: FCVT       r10, r9\n"
    "   14: GET_ALIAS  r11, a3\n"
    "   15: VEXTRACT   r12, r11, 1\n"
    "   16: FCAST      r13, r12\n"
    "   17: FCMP       r14, r10, r13, LT\n"
    "   18: FCMP       r15, r10, r13, GT\n"
    "   19: FCMP       r16, r10, r13, EQ\n"
    "   20: FCMP       r17, r10, r13, UN\n"
    "   21: SET_ALIAS  a5, r14\n"
    "   22: SET_ALIAS  a6, r15\n"
    "   23: SET_ALIAS  a7, r16\n"
    "   24: SET_ALIAS  a8, r17\n"
    "   25: FCVT       r18, r10\n"
    "   26: VBROADCAST r19, r18\n"
    "   27: SET_ALIAS  a2, r19\n"
    "   28: LOAD_IMM   r20, 8\n"
    "   29: SET_ALIAS  a1, r20\n"
    "   30: GET_ALIAS  r21, a4\n"
    "   31: ANDI       r22, r21, -16\n"
    "   32: GET_ALIAS  r23, a5\n"
    "   33: SLLI       r24, r23, 3\n"
    "   34: OR         r25, r22, r24\n"
    "   35: GET_ALIAS  r26, a6\n"
    "   36: SLLI       r27, r26, 2\n"
    "   37: OR         r28, r25, r27\n"
    "   38: GET_ALIAS  r29, a7\n"
    "   39: SLLI       r30, r29, 1\n"
    "   40: OR         r31, r28, r30\n"
    "   41: GET_ALIAS  r32, a8\n"
    "   42: OR         r33, r31, r32\n"
    "   43: SET_ALIAS  a4, r33\n"
    "   44: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,44] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
