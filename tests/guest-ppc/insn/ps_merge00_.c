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
    0x10,0x22,0x1C,0x21,  // ps_merge00. f1,f2,f3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a5\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a6, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a7, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a8, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a9, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: FCAST      r9, r8\n"
    "   13: GET_ALIAS  r10, a4\n"
    "   14: FGETSTATE  r11\n"
    "   15: FSETROUND  r12, r11, TRUNC\n"
    "   16: FSETSTATE  r12\n"
    "   17: FCAST      r13, r10\n"
    "   18: FSETSTATE  r11\n"
    "   19: VBUILD2    r14, r9, r13\n"
    "   20: GET_ALIAS  r15, a10\n"
    "   21: ANDI       r16, r15, 33031936\n"
    "   22: SGTUI      r17, r16, 0\n"
    "   23: BFEXT      r18, r15, 25, 4\n"
    "   24: SLLI       r19, r17, 4\n"
    "   25: SRLI       r20, r15, 3\n"
    "   26: OR         r21, r18, r19\n"
    "   27: AND        r22, r21, r20\n"
    "   28: ANDI       r23, r22, 31\n"
    "   29: SGTUI      r24, r23, 0\n"
    "   30: BFEXT      r25, r15, 31, 1\n"
    "   31: BFEXT      r26, r15, 28, 1\n"
    "   32: SET_ALIAS  a6, r25\n"
    "   33: SET_ALIAS  a7, r24\n"
    "   34: SET_ALIAS  a8, r17\n"
    "   35: SET_ALIAS  a9, r26\n"
    "   36: VFCAST     r27, r14\n"
    "   37: SET_ALIAS  a2, r27\n"
    "   38: LOAD_IMM   r28, 4\n"
    "   39: SET_ALIAS  a1, r28\n"
    "   40: GET_ALIAS  r29, a5\n"
    "   41: ANDI       r30, r29, -251658241\n"
    "   42: GET_ALIAS  r31, a6\n"
    "   43: SLLI       r32, r31, 27\n"
    "   44: OR         r33, r30, r32\n"
    "   45: GET_ALIAS  r34, a7\n"
    "   46: SLLI       r35, r34, 26\n"
    "   47: OR         r36, r33, r35\n"
    "   48: GET_ALIAS  r37, a8\n"
    "   49: SLLI       r38, r37, 25\n"
    "   50: OR         r39, r36, r38\n"
    "   51: GET_ALIAS  r40, a9\n"
    "   52: SLLI       r41, r40, 24\n"
    "   53: OR         r42, r39, r41\n"
    "   54: SET_ALIAS  a5, r42\n"
    "   55: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,55] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
