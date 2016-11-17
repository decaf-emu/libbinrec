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
    0x10,0x22,0x20,0xEF,  // ps_sel. %f1,%f2,%f3,%f4
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
    "   13: GET_ALIAS  r10, a4\n"
    "   14: GET_ALIAS  r11, a5\n"
    "   15: LOAD_IMM   r12, 0.0\n"
    "   16: VEXTRACT   r13, r9, 0\n"
    "   17: VEXTRACT   r14, r10, 0\n"
    "   18: VEXTRACT   r15, r11, 0\n"
    "   19: FCMP       r16, r13, r12, GE\n"
    "   20: SELECT     r17, r14, r15, r16\n"
    "   21: VEXTRACT   r18, r9, 1\n"
    "   22: VEXTRACT   r19, r10, 1\n"
    "   23: VEXTRACT   r20, r11, 1\n"
    "   24: FCMP       r21, r18, r12, GE\n"
    "   25: SELECT     r22, r19, r20, r21\n"
    "   26: VBUILD2    r23, r17, r22\n"
    "   27: FSETSTATE  r8\n"
    "   28: GET_ALIAS  r24, a11\n"
    "   29: ANDI       r25, r24, 33031936\n"
    "   30: SGTUI      r26, r25, 0\n"
    "   31: BFEXT      r27, r24, 25, 4\n"
    "   32: SLLI       r28, r26, 4\n"
    "   33: SRLI       r29, r24, 3\n"
    "   34: OR         r30, r27, r28\n"
    "   35: AND        r31, r30, r29\n"
    "   36: ANDI       r32, r31, 31\n"
    "   37: SGTUI      r33, r32, 0\n"
    "   38: BFEXT      r34, r24, 31, 1\n"
    "   39: BFEXT      r35, r24, 28, 1\n"
    "   40: SET_ALIAS  a7, r34\n"
    "   41: SET_ALIAS  a8, r33\n"
    "   42: SET_ALIAS  a9, r26\n"
    "   43: SET_ALIAS  a10, r35\n"
    "   44: SET_ALIAS  a2, r23\n"
    "   45: LOAD_IMM   r36, 4\n"
    "   46: SET_ALIAS  a1, r36\n"
    "   47: GET_ALIAS  r37, a6\n"
    "   48: ANDI       r38, r37, -251658241\n"
    "   49: GET_ALIAS  r39, a7\n"
    "   50: SLLI       r40, r39, 27\n"
    "   51: OR         r41, r38, r40\n"
    "   52: GET_ALIAS  r42, a8\n"
    "   53: SLLI       r43, r42, 26\n"
    "   54: OR         r44, r41, r43\n"
    "   55: GET_ALIAS  r45, a9\n"
    "   56: SLLI       r46, r45, 25\n"
    "   57: OR         r47, r44, r46\n"
    "   58: GET_ALIAS  r48, a10\n"
    "   59: SLLI       r49, r48, 24\n"
    "   60: OR         r50, r47, r49\n"
    "   61: SET_ALIAS  a6, r50\n"
    "   62: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,62] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
