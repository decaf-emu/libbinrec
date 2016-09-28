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
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: SLTSI      r3, r2, 0\n"
    "    4: SGTSI      r4, r2, 0\n"
    "    5: SEQI       r5, r2, 0\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: BFEXT      r6, r7, 31, 1\n"
    "    8: SLLI       r8, r5, 1\n"
    "    9: OR         r9, r6, r8\n"
    "   10: SLLI       r10, r4, 2\n"
    "   11: OR         r11, r9, r10\n"
    "   12: SLLI       r12, r3, 3\n"
    "   13: OR         r13, r11, r12\n"
    "   14: SET_ALIAS  a2, r2\n"
    "   15: SET_ALIAS  a4, r13\n"
    "   16: LOAD_IMM   r14, 4\n"
    "   17: SET_ALIAS  a1, r14\n"
    "   18: LABEL      L2\n"
    "   19: LOAD       r15, 928(r1)\n"
    "   20: ANDI       r16, r15, 268435455\n"
    "   21: GET_ALIAS  r17, a4\n"
    "   22: SLLI       r18, r17, 28\n"
    "   23: OR         r19, r16, r18\n"
    "   24: STORE      928(r1), r19\n"
    "   25: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,17] --> 2\n"
    "Block 2: 1 --> [18,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
