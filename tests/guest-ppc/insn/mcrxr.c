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
    0x7F,0x80,0x04,0x00,  // mcrxr cr7
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: SRLI       r6, r5, 28\n"
    "    8: ANDI       r7, r5, 268435455\n"
    "    9: SET_ALIAS  a2, r6\n"
    "   10: SET_ALIAS  a3, r7\n"
    "   11: LOAD_IMM   r8, 4\n"
    "   12: SET_ALIAS  a1, r8\n"
    "   13: LABEL      L2\n"
    "   14: LOAD       r9, 928(r1)\n"
    "   15: ANDI       r10, r9, -16\n"
    "   16: GET_ALIAS  r11, a2\n"
    "   17: OR         r12, r10, r11\n"
    "   18: STORE      928(r1), r12\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,12] --> 2\n"
    "Block 2: 1 --> [13,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
