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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: SLTSI      r6, r5, 0\n"
    "    8: SGTSI      r7, r5, 0\n"
    "    9: SEQI       r8, r5, 0\n"
    "   10: GET_ALIAS  r10, a5\n"
    "   11: BFEXT      r9, r10, 31, 1\n"
    "   12: SLLI       r11, r8, 1\n"
    "   13: OR         r12, r9, r11\n"
    "   14: SLLI       r13, r7, 2\n"
    "   15: OR         r14, r12, r13\n"
    "   16: SLLI       r15, r6, 3\n"
    "   17: OR         r16, r14, r15\n"
    "   18: SET_ALIAS  a2, r5\n"
    "   19: SET_ALIAS  a4, r16\n"
    "   20: LOAD_IMM   r17, 4\n"
    "   21: SET_ALIAS  a1, r17\n"
    "   22: LABEL      L2\n"
    "   23: LOAD       r18, 928(r1)\n"
    "   24: ANDI       r19, r18, 268435455\n"
    "   25: GET_ALIAS  r20, a4\n"
    "   26: SLLI       r21, r20, 28\n"
    "   27: OR         r22, r19, r21\n"
    "   28: STORE      928(r1), r22\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,21] --> 2\n"
    "Block 2: 1 --> [22,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
