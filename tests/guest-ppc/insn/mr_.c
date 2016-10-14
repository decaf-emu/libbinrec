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
    0x7C,0x83,0x23,0x79,  // mr. r3,r4 (or. r3,r4,r4)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: SLTSI      r4, r3, 0\n"
    "    5: SGTSI      r5, r3, 0\n"
    "    6: SEQI       r6, r3, 0\n"
    "    7: GET_ALIAS  r7, a9\n"
    "    8: BFEXT      r8, r7, 31, 1\n"
    "    9: SET_ALIAS  a2, r3\n"
    "   10: SET_ALIAS  a4, r4\n"
    "   11: SET_ALIAS  a5, r5\n"
    "   12: SET_ALIAS  a6, r6\n"
    "   13: SET_ALIAS  a7, r8\n"
    "   14: LOAD_IMM   r9, 4\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L2\n"
    "   17: GET_ALIAS  r10, a8\n"
    "   18: ANDI       r11, r10, 268435455\n"
    "   19: GET_ALIAS  r12, a4\n"
    "   20: SLLI       r13, r12, 31\n"
    "   21: OR         r14, r11, r13\n"
    "   22: GET_ALIAS  r15, a5\n"
    "   23: SLLI       r16, r15, 30\n"
    "   24: OR         r17, r14, r16\n"
    "   25: GET_ALIAS  r18, a6\n"
    "   26: SLLI       r19, r18, 29\n"
    "   27: OR         r20, r17, r19\n"
    "   28: GET_ALIAS  r21, a7\n"
    "   29: SLLI       r22, r21, 28\n"
    "   30: OR         r23, r20, r22\n"
    "   31: SET_ALIAS  a8, r23\n"
    "   32: RETURN\n"
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
    "Block 1: 0 --> [2,15] --> 2\n"
    "Block 2: 1 --> [16,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
