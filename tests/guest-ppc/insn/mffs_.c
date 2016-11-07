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
    0xFC,0x20,0x04,0x8F,  // mffs. f1
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a8\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a9, r4\n"
    "    5: GET_ALIAS  r5, a8\n"
    "    6: ANDI       r6, r5, 33031936\n"
    "    7: SGTUI      r7, r6, 0\n"
    "    8: BFEXT      r8, r5, 25, 4\n"
    "    9: SLLI       r9, r7, 4\n"
    "   10: SRLI       r10, r5, 3\n"
    "   11: OR         r11, r8, r9\n"
    "   12: AND        r12, r11, r10\n"
    "   13: ANDI       r13, r12, 31\n"
    "   14: SGTUI      r14, r13, 0\n"
    "   15: GET_ALIAS  r15, a9\n"
    "   16: ANDI       r16, r5, -1611134977\n"
    "   17: SLLI       r17, r15, 12\n"
    "   18: OR         r18, r16, r17\n"
    "   19: SLLI       r19, r14, 30\n"
    "   20: SLLI       r20, r7, 29\n"
    "   21: OR         r21, r19, r20\n"
    "   22: OR         r22, r18, r21\n"
    "   23: ZCAST      r23, r22\n"
    "   24: BITCAST    r24, r23\n"
    "   25: BFEXT      r25, r18, 31, 1\n"
    "   26: BFEXT      r26, r18, 28, 1\n"
    "   27: SET_ALIAS  a3, r25\n"
    "   28: SET_ALIAS  a4, r14\n"
    "   29: SET_ALIAS  a5, r7\n"
    "   30: SET_ALIAS  a6, r26\n"
    "   31: SET_ALIAS  a2, r24\n"
    "   32: LOAD_IMM   r27, 4\n"
    "   33: SET_ALIAS  a1, r27\n"
    "   34: GET_ALIAS  r28, a7\n"
    "   35: ANDI       r29, r28, -251658241\n"
    "   36: GET_ALIAS  r30, a3\n"
    "   37: SLLI       r31, r30, 27\n"
    "   38: OR         r32, r29, r31\n"
    "   39: GET_ALIAS  r33, a4\n"
    "   40: SLLI       r34, r33, 26\n"
    "   41: OR         r35, r32, r34\n"
    "   42: GET_ALIAS  r36, a5\n"
    "   43: SLLI       r37, r36, 25\n"
    "   44: OR         r38, r35, r37\n"
    "   45: GET_ALIAS  r39, a6\n"
    "   46: SLLI       r40, r39, 24\n"
    "   47: OR         r41, r38, r40\n"
    "   48: SET_ALIAS  a7, r41\n"
    "   49: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 928(r1)\n"
    "Alias 8: int32 @ 944(r1)\n"
    "Alias 9: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,49] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
