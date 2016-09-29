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
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: SRLI       r4, r3, 28\n"
    "    5: ANDI       r5, r3, 268435455\n"
    "    6: SET_ALIAS  a2, r4\n"
    "    7: SET_ALIAS  a3, r5\n"
    "    8: LOAD_IMM   r6, 4\n"
    "    9: SET_ALIAS  a1, r6\n"
    "   10: LABEL      L2\n"
    "   11: LOAD       r7, 928(r1)\n"
    "   12: ANDI       r8, r7, -16\n"
    "   13: GET_ALIAS  r9, a2\n"
    "   14: OR         r10, r8, r9\n"
    "   15: STORE      928(r1), r10\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,9] --> 2\n"
    "Block 2: 1 --> [10,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
