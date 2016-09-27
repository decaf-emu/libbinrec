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
    0x34,0x60,0x12,0x34,  // addic. r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: ADDI       r3, r2, 4660\n"
    "    4: GET_ALIAS  r4, a5\n"
    "    5: SLTUI      r5, r3, 4660\n"
    "    6: BFINS      r6, r4, r5, 29, 1\n"
    "    7: SLTSI      r7, r3, 0\n"
    "    8: SGTSI      r8, r3, 0\n"
    "    9: SEQI       r9, r3, 0\n"
    "   10: BFEXT      r10, r6, 31, 1\n"
    "   11: SLLI       r11, r9, 1\n"
    "   12: OR         r12, r10, r11\n"
    "   13: SLLI       r13, r8, 2\n"
    "   14: OR         r14, r12, r13\n"
    "   15: SLLI       r15, r7, 3\n"
    "   16: OR         r16, r14, r15\n"
    "   17: SET_ALIAS  a3, r3\n"
    "   18: SET_ALIAS  a4, r16\n"
    "   19: SET_ALIAS  a5, r6\n"
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
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,21] --> 2\n"
    "Block 2: 1 --> [22,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
