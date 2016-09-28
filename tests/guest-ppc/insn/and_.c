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
    0x7C,0x83,0x28,0x39,  // and. r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: AND        r4, r2, r3\n"
    "    5: SLTSI      r5, r4, 0\n"
    "    6: SGTSI      r6, r4, 0\n"
    "    7: SEQI       r7, r4, 0\n"
    "    8: GET_ALIAS  r9, a6\n"
    "    9: BFEXT      r8, r9, 31, 1\n"
    "   10: SLLI       r10, r7, 1\n"
    "   11: OR         r11, r8, r10\n"
    "   12: SLLI       r12, r6, 2\n"
    "   13: OR         r13, r11, r12\n"
    "   14: SLLI       r14, r5, 3\n"
    "   15: OR         r15, r13, r14\n"
    "   16: SET_ALIAS  a2, r4\n"
    "   17: SET_ALIAS  a5, r15\n"
    "   18: LOAD_IMM   r16, 4\n"
    "   19: SET_ALIAS  a1, r16\n"
    "   20: LABEL      L2\n"
    "   21: LOAD       r17, 928(r1)\n"
    "   22: ANDI       r18, r17, 268435455\n"
    "   23: GET_ALIAS  r19, a5\n"
    "   24: SLLI       r20, r19, 28\n"
    "   25: OR         r21, r18, r20\n"
    "   26: STORE      928(r1), r21\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,19] --> 2\n"
    "Block 2: 1 --> [20,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
