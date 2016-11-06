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
    0xFC,0x00,0x00,0x4D,  // mtfsb1. 0
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a8, r4\n"
    "    5: GET_ALIAS  r5, a7\n"
    "    6: ORI        r6, r5, -2147483648\n"
    "    7: SET_ALIAS  a7, r6\n"
    "    8: ANDI       r7, r6, 33031936\n"
    "    9: SGTUI      r8, r7, 0\n"
    "   10: BFEXT      r9, r6, 25, 4\n"
    "   11: SLLI       r10, r8, 4\n"
    "   12: SRLI       r11, r6, 3\n"
    "   13: OR         r12, r9, r10\n"
    "   14: AND        r13, r12, r11\n"
    "   15: ANDI       r14, r13, 31\n"
    "   16: SGTUI      r15, r14, 0\n"
    "   17: BFEXT      r16, r6, 31, 1\n"
    "   18: BFEXT      r17, r6, 28, 1\n"
    "   19: SET_ALIAS  a2, r16\n"
    "   20: SET_ALIAS  a3, r15\n"
    "   21: SET_ALIAS  a4, r8\n"
    "   22: SET_ALIAS  a5, r17\n"
    "   23: LOAD_IMM   r18, 4\n"
    "   24: SET_ALIAS  a1, r18\n"
    "   25: GET_ALIAS  r19, a6\n"
    "   26: ANDI       r20, r19, -251658241\n"
    "   27: GET_ALIAS  r21, a2\n"
    "   28: SLLI       r22, r21, 27\n"
    "   29: OR         r23, r20, r22\n"
    "   30: GET_ALIAS  r24, a3\n"
    "   31: SLLI       r25, r24, 26\n"
    "   32: OR         r26, r23, r25\n"
    "   33: GET_ALIAS  r27, a4\n"
    "   34: SLLI       r28, r27, 25\n"
    "   35: OR         r29, r26, r28\n"
    "   36: GET_ALIAS  r30, a5\n"
    "   37: SLLI       r31, r30, 24\n"
    "   38: OR         r32, r29, r31\n"
    "   39: SET_ALIAS  a6, r32\n"
    "   40: GET_ALIAS  r33, a7\n"
    "   41: GET_ALIAS  r34, a8\n"
    "   42: ANDI       r35, r33, -1611134977\n"
    "   43: SLLI       r36, r34, 12\n"
    "   44: OR         r37, r35, r36\n"
    "   45: SET_ALIAS  a7, r37\n"
    "   46: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "Alias 8: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,46] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
