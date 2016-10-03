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
    0x2B,0x80,0xAB,0xCD,  // cmpli cr7,r0,43981
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: SLTUI      r6, r5, 43981\n"
    "    8: SGTUI      r7, r5, 43981\n"
    "    9: SEQI       r8, r5, 43981\n"
    "   10: GET_ALIAS  r10, a4\n"
    "   11: BFEXT      r9, r10, 31, 1\n"
    "   12: SLLI       r11, r8, 1\n"
    "   13: OR         r12, r9, r11\n"
    "   14: SLLI       r13, r7, 2\n"
    "   15: OR         r14, r12, r13\n"
    "   16: SLLI       r15, r6, 3\n"
    "   17: OR         r16, r14, r15\n"
    "   18: SET_ALIAS  a3, r16\n"
    "   19: LOAD_IMM   r17, 4\n"
    "   20: SET_ALIAS  a1, r17\n"
    "   21: LABEL      L2\n"
    "   22: LOAD       r18, 928(r1)\n"
    "   23: ANDI       r19, r18, -16\n"
    "   24: GET_ALIAS  r20, a3\n"
    "   25: OR         r21, r19, r20\n"
    "   26: STORE      928(r1), r21\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,20] --> 2\n"
    "Block 2: 1 --> [21,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
