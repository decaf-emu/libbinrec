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

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a8\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a9, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: GET_ALIAS  r8, a8\n"
    "    9: ANDI       r9, r8, 33031936\n"
    "   10: SGTUI      r10, r9, 0\n"
    "   11: BFEXT      r11, r8, 25, 4\n"
    "   12: SLLI       r12, r10, 4\n"
    "   13: SRLI       r13, r8, 3\n"
    "   14: OR         r14, r11, r12\n"
    "   15: AND        r15, r14, r13\n"
    "   16: ANDI       r16, r15, 31\n"
    "   17: SGTUI      r17, r16, 0\n"
    "   18: BFEXT      r18, r8, 31, 1\n"
    "   19: BFEXT      r19, r8, 28, 1\n"
    "   20: SET_ALIAS  a3, r18\n"
    "   21: SET_ALIAS  a4, r17\n"
    "   22: SET_ALIAS  a5, r10\n"
    "   23: SET_ALIAS  a6, r19\n"
    "   24: LOAD_IMM   r20, 4\n"
    "   25: SET_ALIAS  a1, r20\n"
    "   26: GET_ALIAS  r21, a7\n"
    "   27: ANDI       r22, r21, -251658241\n"
    "   28: GET_ALIAS  r23, a3\n"
    "   29: SLLI       r24, r23, 27\n"
    "   30: OR         r25, r22, r24\n"
    "   31: GET_ALIAS  r26, a4\n"
    "   32: SLLI       r27, r26, 26\n"
    "   33: OR         r28, r25, r27\n"
    "   34: GET_ALIAS  r29, a5\n"
    "   35: SLLI       r30, r29, 25\n"
    "   36: OR         r31, r28, r30\n"
    "   37: GET_ALIAS  r32, a6\n"
    "   38: SLLI       r33, r32, 24\n"
    "   39: OR         r34, r31, r33\n"
    "   40: SET_ALIAS  a7, r34\n"
    "   41: GET_ALIAS  r35, a8\n"
    "   42: GET_ALIAS  r36, a9\n"
    "   43: ANDI       r37, r35, -1611134977\n"
    "   44: SLLI       r38, r36, 12\n"
    "   45: OR         r39, r37, r38\n"
    "   46: SET_ALIAS  a8, r39\n"
    "   47: RETURN\n"
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
    "Block 0: <none> --> [0,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
