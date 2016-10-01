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
    0x54,0x83,0x29,0x8F,  // rlwinm. r3,r4,5,6,7
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: RORI       r4, r3, 27\n"
    "    5: ANDI       r5, r4, 50331648\n"
    "    6: SLTSI      r6, r5, 0\n"
    "    7: SGTSI      r7, r5, 0\n"
    "    8: SEQI       r8, r5, 0\n"
    "    9: GET_ALIAS  r10, a5\n"
    "   10: BFEXT      r9, r10, 31, 1\n"
    "   11: SLLI       r11, r8, 1\n"
    "   12: OR         r12, r9, r11\n"
    "   13: SLLI       r13, r7, 2\n"
    "   14: OR         r14, r12, r13\n"
    "   15: SLLI       r15, r6, 3\n"
    "   16: OR         r16, r14, r15\n"
    "   17: SET_ALIAS  a2, r5\n"
    "   18: SET_ALIAS  a4, r16\n"
    "   19: LOAD_IMM   r17, 4\n"
    "   20: SET_ALIAS  a1, r17\n"
    "   21: LABEL      L2\n"
    "   22: LOAD       r18, 928(r1)\n"
    "   23: ANDI       r19, r18, 268435455\n"
    "   24: GET_ALIAS  r20, a4\n"
    "   25: SLLI       r21, r20, 28\n"
    "   26: OR         r22, r19, r21\n"
    "   27: STORE      928(r1), r22\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,20] --> 2\n"
    "Block 2: 1 --> [21,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
