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
    "   14: GET_ALIAS  r10, a8\n"
    "   15: ANDI       r11, r10, 33031936\n"
    "   16: SGTUI      r12, r11, 0\n"
    "   17: BFEXT      r13, r10, 25, 4\n"
    "   18: SLLI       r14, r12, 4\n"
    "   19: SRLI       r15, r10, 3\n"
    "   20: OR         r16, r13, r14\n"
    "   21: AND        r17, r16, r15\n"
    "   22: ANDI       r18, r17, 31\n"
    "   23: SGTUI      r19, r18, 0\n"
    "   24: GET_ALIAS  r20, a9\n"
    "   25: ANDI       r21, r10, -1611134977\n"
    "   26: SLLI       r22, r20, 12\n"
    "   27: OR         r23, r21, r22\n"
    "   28: SLLI       r24, r19, 30\n"
    "   29: SLLI       r25, r12, 29\n"
    "   30: OR         r26, r24, r25\n"
    "   31: OR         r27, r23, r26\n"
    "   32: ZCAST      r28, r27\n"
    "   33: BITCAST    r29, r28\n"
    "   34: BFEXT      r30, r23, 31, 1\n"
    "   35: BFEXT      r31, r23, 28, 1\n"
    "   36: SET_ALIAS  a4, r30\n"
    "   37: SET_ALIAS  a5, r19\n"
    "   38: SET_ALIAS  a6, r12\n"
    "   39: SET_ALIAS  a7, r31\n"
    "   40: SET_ALIAS  a2, r29\n"
    "   41: LOAD_IMM   r32, 4\n"
    "   42: SET_ALIAS  a1, r32\n"
    "   43: GET_ALIAS  r33, a3\n"
    "   44: ANDI       r34, r33, -251658241\n"
    "   45: GET_ALIAS  r35, a4\n"
    "   46: SLLI       r36, r35, 27\n"
    "   47: OR         r37, r34, r36\n"
    "   48: GET_ALIAS  r38, a5\n"
    "   49: SLLI       r39, r38, 26\n"
    "   50: OR         r40, r37, r39\n"
    "   51: GET_ALIAS  r41, a6\n"
    "   52: SLLI       r42, r41, 25\n"
    "   53: OR         r43, r40, r42\n"
    "   54: GET_ALIAS  r44, a7\n"
    "   55: SLLI       r45, r44, 24\n"
    "   56: OR         r46, r43, r45\n"
    "   57: SET_ALIAS  a3, r46\n"
    "   58: RETURN\n"
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
    "Block 0: <none> --> [0,58] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
