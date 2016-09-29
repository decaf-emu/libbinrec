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
    "    7: GET_ALIAS  r8, a5\n"
    "    8: BFEXT      r7, r8, 31, 1\n"
    "    9: SLLI       r9, r6, 1\n"
    "   10: OR         r10, r7, r9\n"
    "   11: SLLI       r11, r5, 2\n"
    "   12: OR         r12, r10, r11\n"
    "   13: SLLI       r13, r4, 3\n"
    "   14: OR         r14, r12, r13\n"
    "   15: SET_ALIAS  a2, r3\n"
    "   16: SET_ALIAS  a4, r14\n"
    "   17: LOAD_IMM   r15, 4\n"
    "   18: SET_ALIAS  a1, r15\n"
    "   19: LABEL      L2\n"
    "   20: LOAD       r16, 928(r1)\n"
    "   21: ANDI       r17, r16, 268435455\n"
    "   22: GET_ALIAS  r18, a4\n"
    "   23: SLLI       r19, r18, 28\n"
    "   24: OR         r20, r17, r19\n"
    "   25: STORE      928(r1), r20\n"
    "   26: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,18] --> 2\n"
    "Block 2: 1 --> [19,26] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
