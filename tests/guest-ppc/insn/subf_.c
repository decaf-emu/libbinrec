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
    0x7C,0x64,0x28,0x51,  // subf. r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: GET_ALIAS  r6, a4\n"
    "    8: SUB        r7, r6, r5\n"
    "    9: SLTSI      r8, r7, 0\n"
    "   10: SGTSI      r9, r7, 0\n"
    "   11: SEQI       r10, r7, 0\n"
    "   12: GET_ALIAS  r12, a6\n"
    "   13: BFEXT      r11, r12, 31, 1\n"
    "   14: SLLI       r13, r10, 1\n"
    "   15: OR         r14, r11, r13\n"
    "   16: SLLI       r15, r9, 2\n"
    "   17: OR         r16, r14, r15\n"
    "   18: SLLI       r17, r8, 3\n"
    "   19: OR         r18, r16, r17\n"
    "   20: SET_ALIAS  a2, r7\n"
    "   21: SET_ALIAS  a5, r18\n"
    "   22: LOAD_IMM   r19, 4\n"
    "   23: SET_ALIAS  a1, r19\n"
    "   24: LABEL      L2\n"
    "   25: LOAD       r20, 928(r1)\n"
    "   26: ANDI       r21, r20, 268435455\n"
    "   27: GET_ALIAS  r22, a5\n"
    "   28: SLLI       r23, r22, 28\n"
    "   29: OR         r24, r21, r23\n"
    "   30: STORE      928(r1), r24\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,23] --> 2\n"
    "Block 2: 1 --> [24,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
