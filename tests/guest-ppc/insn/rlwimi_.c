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
    "   10: GET_ALIAS  r11, a5\n"
    "   11: BFEXT      r10, r11, 31, 1\n"
    "   12: SLLI       r12, r9, 1\n"
    "   13: OR         r13, r10, r12\n"
    "   14: SLLI       r14, r8, 2\n"
    "   15: OR         r15, r13, r14\n"
    "   16: SLLI       r16, r7, 3\n"
    "   17: OR         r17, r15, r16\n"
    "   18: SET_ALIAS  a2, r6\n"
    "   19: SET_ALIAS  a4, r17\n"
    "   20: LOAD_IMM   r18, 4\n"
    "   21: SET_ALIAS  a1, r18\n"
    "   22: LABEL      L2\n"
    "   23: LOAD       r19, 928(r1)\n"
    "   24: ANDI       r20, r19, 268435455\n"
    "   25: GET_ALIAS  r21, a4\n"
    "   26: SLLI       r22, r21, 28\n"
    "   27: OR         r23, r20, r22\n"
    "   28: STORE      928(r1), r23\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,21] --> 2\n"
    "Block 2: 1 --> [22,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
