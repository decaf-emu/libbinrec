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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: GET_ALIAS  r6, a4\n"
    "    8: ANDI       r7, r6, 63\n"
    "    9: ZCAST      r8, r5\n"
    "   10: SLL        r9, r8, r7\n"
    "   11: ZCAST      r10, r9\n"
    "   12: SLTSI      r11, r10, 0\n"
    "   13: SGTSI      r12, r10, 0\n"
    "   14: SEQI       r13, r10, 0\n"
    "   15: GET_ALIAS  r15, a6\n"
    "   16: BFEXT      r14, r15, 31, 1\n"
    "   17: SLLI       r16, r13, 1\n"
    "   18: OR         r17, r14, r16\n"
    "   19: SLLI       r18, r12, 2\n"
    "   20: OR         r19, r17, r18\n"
    "   21: SLLI       r20, r11, 3\n"
    "   22: OR         r21, r19, r20\n"
    "   23: SET_ALIAS  a2, r10\n"
    "   24: SET_ALIAS  a5, r21\n"
    "   25: LOAD_IMM   r22, 4\n"
    "   26: SET_ALIAS  a1, r22\n"
    "   27: LABEL      L2\n"
    "   28: LOAD       r23, 928(r1)\n"
    "   29: ANDI       r24, r23, 268435455\n"
    "   30: GET_ALIAS  r25, a5\n"
    "   31: SLLI       r26, r25, 28\n"
    "   32: OR         r27, r24, r26\n"
    "   33: STORE      928(r1), r27\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,26] --> 2\n"
    "Block 2: 1 --> [27,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
