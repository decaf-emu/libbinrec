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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: GET_ALIAS  r6, a2\n"
    "    8: RORI       r7, r5, 19\n"
    "    9: BFINS      r8, r6, r7, 24, 2\n"
    "   10: SLTSI      r9, r8, 0\n"
    "   11: SGTSI      r10, r8, 0\n"
    "   12: SEQI       r11, r8, 0\n"
    "   13: GET_ALIAS  r13, a5\n"
    "   14: BFEXT      r12, r13, 31, 1\n"
    "   15: SLLI       r14, r11, 1\n"
    "   16: OR         r15, r12, r14\n"
    "   17: SLLI       r16, r10, 2\n"
    "   18: OR         r17, r15, r16\n"
    "   19: SLLI       r18, r9, 3\n"
    "   20: OR         r19, r17, r18\n"
    "   21: SET_ALIAS  a2, r8\n"
    "   22: SET_ALIAS  a4, r19\n"
    "   23: LOAD_IMM   r20, 4\n"
    "   24: SET_ALIAS  a1, r20\n"
    "   25: LABEL      L2\n"
    "   26: LOAD       r21, 928(r1)\n"
    "   27: ANDI       r22, r21, 268435455\n"
    "   28: GET_ALIAS  r23, a4\n"
    "   29: SLLI       r24, r23, 28\n"
    "   30: OR         r25, r22, r24\n"
    "   31: STORE      928(r1), r25\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,24] --> 2\n"
    "Block 2: 1 --> [25,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
