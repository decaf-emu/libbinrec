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
    0x70,0x03,0x12,0x34,  // andi. r3,r0,4660
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
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ANDI       r6, r5, 4660\n"
    "    8: SLTSI      r7, r6, 0\n"
    "    9: SGTSI      r8, r6, 0\n"
    "   10: SEQI       r9, r6, 0\n"
    "   11: GET_ALIAS  r11, a5\n"
    "   12: BFEXT      r10, r11, 31, 1\n"
    "   13: SLLI       r12, r9, 1\n"
    "   14: OR         r13, r10, r12\n"
    "   15: SLLI       r14, r8, 2\n"
    "   16: OR         r15, r13, r14\n"
    "   17: SLLI       r16, r7, 3\n"
    "   18: OR         r17, r15, r16\n"
    "   19: SET_ALIAS  a3, r6\n"
    "   20: SET_ALIAS  a4, r17\n"
    "   21: LOAD_IMM   r18, 4\n"
    "   22: SET_ALIAS  a1, r18\n"
    "   23: LABEL      L2\n"
    "   24: LOAD       r19, 928(r1)\n"
    "   25: ANDI       r20, r19, 268435455\n"
    "   26: GET_ALIAS  r21, a4\n"
    "   27: SLLI       r22, r21, 28\n"
    "   28: OR         r23, r20, r22\n"
    "   29: STORE      928(r1), r23\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,22] --> 2\n"
    "Block 2: 1 --> [23,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
