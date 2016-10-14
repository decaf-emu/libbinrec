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
    0x50,0x83,0x29,0x8F,  // rlwimi. r3,r4,5,6,7
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a2\n"
    "    5: RORI       r5, r3, 19\n"
    "    6: BFINS      r6, r4, r5, 24, 2\n"
    "    7: SLTSI      r7, r6, 0\n"
    "    8: SGTSI      r8, r6, 0\n"
    "    9: SEQI       r9, r6, 0\n"
    "   10: GET_ALIAS  r10, a9\n"
    "   11: BFEXT      r11, r10, 31, 1\n"
    "   12: SET_ALIAS  a2, r6\n"
    "   13: SET_ALIAS  a4, r7\n"
    "   14: SET_ALIAS  a5, r8\n"
    "   15: SET_ALIAS  a6, r9\n"
    "   16: SET_ALIAS  a7, r11\n"
    "   17: LOAD_IMM   r12, 4\n"
    "   18: SET_ALIAS  a1, r12\n"
    "   19: LABEL      L2\n"
    "   20: GET_ALIAS  r13, a8\n"
    "   21: ANDI       r14, r13, 268435455\n"
    "   22: GET_ALIAS  r15, a4\n"
    "   23: SLLI       r16, r15, 31\n"
    "   24: OR         r17, r14, r16\n"
    "   25: GET_ALIAS  r18, a5\n"
    "   26: SLLI       r19, r18, 30\n"
    "   27: OR         r20, r17, r19\n"
    "   28: GET_ALIAS  r21, a6\n"
    "   29: SLLI       r22, r21, 29\n"
    "   30: OR         r23, r20, r22\n"
    "   31: GET_ALIAS  r24, a7\n"
    "   32: SLLI       r25, r24, 28\n"
    "   33: OR         r26, r23, r25\n"
    "   34: SET_ALIAS  a8, r26\n"
    "   35: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,18] --> 2\n"
    "Block 2: 1 --> [19,35] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
