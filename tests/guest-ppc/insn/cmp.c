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
    0x7F,0x83,0x20,0x00,  // cmp cr7,r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: GET_ALIAS  r9, a3\n"
    "    8: SLTS       r6, r5, r9\n"
    "    9: SGTS       r7, r5, r9\n"
    "   10: SEQ        r8, r5, r9\n"
    "   11: GET_ALIAS  r11, a5\n"
    "   12: BFEXT      r10, r11, 31, 1\n"
    "   13: SLLI       r12, r8, 1\n"
    "   14: OR         r13, r10, r12\n"
    "   15: SLLI       r14, r7, 2\n"
    "   16: OR         r15, r13, r14\n"
    "   17: SLLI       r16, r6, 3\n"
    "   18: OR         r17, r15, r16\n"
    "   19: SET_ALIAS  a4, r17\n"
    "   20: LOAD_IMM   r18, 4\n"
    "   21: SET_ALIAS  a1, r18\n"
    "   22: LABEL      L2\n"
    "   23: LOAD       r19, 928(r1)\n"
    "   24: ANDI       r20, r19, -16\n"
    "   25: GET_ALIAS  r21, a4\n"
    "   26: OR         r22, r20, r21\n"
    "   27: STORE      928(r1), r22\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,21] --> 2\n"
    "Block 2: 1 --> [22,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
