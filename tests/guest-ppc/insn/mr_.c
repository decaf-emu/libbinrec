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
    "    2: GET_ALIAS  r3, a3\n"
    "    3: SLTSI      r4, r3, 0\n"
    "    4: SGTSI      r5, r3, 0\n"
    "    5: SEQI       r6, r3, 0\n"
    "    6: GET_ALIAS  r7, a9\n"
    "    7: BFEXT      r8, r7, 31, 1\n"
    "    8: SET_ALIAS  a2, r3\n"
    "    9: SET_ALIAS  a4, r4\n"
    "   10: SET_ALIAS  a5, r5\n"
    "   11: SET_ALIAS  a6, r6\n"
    "   12: SET_ALIAS  a7, r8\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: GET_ALIAS  r10, a8\n"
    "   16: ANDI       r11, r10, 268435455\n"
    "   17: GET_ALIAS  r12, a4\n"
    "   18: SLLI       r13, r12, 31\n"
    "   19: OR         r14, r11, r13\n"
    "   20: GET_ALIAS  r15, a5\n"
    "   21: SLLI       r16, r15, 30\n"
    "   22: OR         r17, r14, r16\n"
    "   23: GET_ALIAS  r18, a6\n"
    "   24: SLLI       r19, r18, 29\n"
    "   25: OR         r20, r17, r19\n"
    "   26: GET_ALIAS  r21, a7\n"
    "   27: SLLI       r22, r21, 28\n"
    "   28: OR         r23, r20, r22\n"
    "   29: SET_ALIAS  a8, r23\n"
    "   30: RETURN\n"
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
    "Block 0: <none> --> [0,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
