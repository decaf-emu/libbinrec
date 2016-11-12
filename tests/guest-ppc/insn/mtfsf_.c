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
    0xFC,0x00,0x0D,0x8F,  // mtfsf. 0,f1
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a5, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a6, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a7, r7\n"
    "   11: GET_ALIAS  r8, a8\n"
    "   12: BFEXT      r9, r8, 12, 7\n"
    "   13: SET_ALIAS  a9, r9\n"
    "   14: GET_ALIAS  r10, a2\n"
    "   15: BITCAST    r11, r10\n"
    "   16: ZCAST      r12, r11\n"
    "   17: GET_ALIAS  r13, a8\n"
    "   18: ANDI       r14, r13, 33031936\n"
    "   19: SGTUI      r15, r14, 0\n"
    "   20: BFEXT      r16, r13, 25, 4\n"
    "   21: SLLI       r17, r15, 4\n"
    "   22: SRLI       r18, r13, 3\n"
    "   23: OR         r19, r16, r17\n"
    "   24: AND        r20, r19, r18\n"
    "   25: ANDI       r21, r20, 31\n"
    "   26: SGTUI      r22, r21, 0\n"
    "   27: BFEXT      r23, r13, 31, 1\n"
    "   28: BFEXT      r24, r13, 28, 1\n"
    "   29: SET_ALIAS  a4, r23\n"
    "   30: SET_ALIAS  a5, r22\n"
    "   31: SET_ALIAS  a6, r15\n"
    "   32: SET_ALIAS  a7, r24\n"
    "   33: LOAD_IMM   r25, 4\n"
    "   34: SET_ALIAS  a1, r25\n"
    "   35: GET_ALIAS  r26, a3\n"
    "   36: ANDI       r27, r26, -251658241\n"
    "   37: GET_ALIAS  r28, a4\n"
    "   38: SLLI       r29, r28, 27\n"
    "   39: OR         r30, r27, r29\n"
    "   40: GET_ALIAS  r31, a5\n"
    "   41: SLLI       r32, r31, 26\n"
    "   42: OR         r33, r30, r32\n"
    "   43: GET_ALIAS  r34, a6\n"
    "   44: SLLI       r35, r34, 25\n"
    "   45: OR         r36, r33, r35\n"
    "   46: GET_ALIAS  r37, a7\n"
    "   47: SLLI       r38, r37, 24\n"
    "   48: OR         r39, r36, r38\n"
    "   49: SET_ALIAS  a3, r39\n"
    "   50: GET_ALIAS  r40, a8\n"
    "   51: GET_ALIAS  r41, a9\n"
    "   52: ANDI       r42, r40, -1611134977\n"
    "   53: SLLI       r43, r41, 12\n"
    "   54: OR         r44, r42, r43\n"
    "   55: SET_ALIAS  a8, r44\n"
    "   56: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 944(r1)\n"
    "Alias 9: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,56] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
