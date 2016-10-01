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
    0x7C,0x83,0x28,0x31,  // slw. r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: ANDI       r5, r4, 63\n"
    "    6: ZCAST      r6, r3\n"
    "    7: SLL        r7, r6, r5\n"
    "    8: ZCAST      r8, r7\n"
    "    9: SLTSI      r9, r8, 0\n"
    "   10: SGTSI      r10, r8, 0\n"
    "   11: SEQI       r11, r8, 0\n"
    "   12: GET_ALIAS  r13, a6\n"
    "   13: BFEXT      r12, r13, 31, 1\n"
    "   14: SLLI       r14, r11, 1\n"
    "   15: OR         r15, r12, r14\n"
    "   16: SLLI       r16, r10, 2\n"
    "   17: OR         r17, r15, r16\n"
    "   18: SLLI       r18, r9, 3\n"
    "   19: OR         r19, r17, r18\n"
    "   20: SET_ALIAS  a2, r8\n"
    "   21: SET_ALIAS  a5, r19\n"
    "   22: LOAD_IMM   r20, 4\n"
    "   23: SET_ALIAS  a1, r20\n"
    "   24: LABEL      L2\n"
    "   25: LOAD       r21, 928(r1)\n"
    "   26: ANDI       r22, r21, 268435455\n"
    "   27: GET_ALIAS  r23, a5\n"
    "   28: SLLI       r24, r23, 28\n"
    "   29: OR         r25, r22, r24\n"
    "   30: STORE      928(r1), r25\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,23] --> 2\n"
    "Block 2: 1 --> [24,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
