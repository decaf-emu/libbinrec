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
    0xFC,0x22,0x20,0xEF,  // fsel. f1,f2,f3,f4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a6\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a7, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a8, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a9, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a10, r7\n"
    "   11: FGETSTATE  r8\n"
    "   12: GET_ALIAS  r9, a3\n"
    "   13: LOAD_IMM   r10, 0.0\n"
    "   14: GET_ALIAS  r11, a4\n"
    "   15: GET_ALIAS  r12, a5\n"
    "   16: FCMP       r13, r9, r10, GE\n"
    "   17: SELECT     r14, r11, r12, r13\n"
    "   18: FSETSTATE  r8\n"
    "   19: GET_ALIAS  r15, a11\n"
    "   20: ANDI       r16, r15, 33031936\n"
    "   21: SGTUI      r17, r16, 0\n"
    "   22: BFEXT      r18, r15, 25, 4\n"
    "   23: SLLI       r19, r17, 4\n"
    "   24: SRLI       r20, r15, 3\n"
    "   25: OR         r21, r18, r19\n"
    "   26: AND        r22, r21, r20\n"
    "   27: ANDI       r23, r22, 31\n"
    "   28: SGTUI      r24, r23, 0\n"
    "   29: BFEXT      r25, r15, 31, 1\n"
    "   30: BFEXT      r26, r15, 28, 1\n"
    "   31: SET_ALIAS  a7, r25\n"
    "   32: SET_ALIAS  a8, r24\n"
    "   33: SET_ALIAS  a9, r17\n"
    "   34: SET_ALIAS  a10, r26\n"
    "   35: SET_ALIAS  a2, r14\n"
    "   36: LOAD_IMM   r27, 4\n"
    "   37: SET_ALIAS  a1, r27\n"
    "   38: GET_ALIAS  r28, a6\n"
    "   39: ANDI       r29, r28, -251658241\n"
    "   40: GET_ALIAS  r30, a7\n"
    "   41: SLLI       r31, r30, 27\n"
    "   42: OR         r32, r29, r31\n"
    "   43: GET_ALIAS  r33, a8\n"
    "   44: SLLI       r34, r33, 26\n"
    "   45: OR         r35, r32, r34\n"
    "   46: GET_ALIAS  r36, a9\n"
    "   47: SLLI       r37, r36, 25\n"
    "   48: OR         r38, r35, r37\n"
    "   49: GET_ALIAS  r39, a10\n"
    "   50: SLLI       r40, r39, 24\n"
    "   51: OR         r41, r38, r40\n"
    "   52: SET_ALIAS  a6, r41\n"
    "   53: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,53] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
