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
    0x1C, 0x60, 0x12, 0x34,  // mulli r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[warning] Scanning terminated at 0x4 due to code range limit\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_I32   r2, 256(r1)\n"
    "    2: SET_ALIAS  a2, r2\n"
    "    3: LABEL      L1\n"
    "    4: GET_ALIAS  r3, a2\n"
    "    5: LOAD_IMM   r4, 4660\n"
    "    6: MUL        r5, r4, r3\n"
    "    7: SET_ALIAS  a3, r5\n"
    "    8: LOAD_IMM   r6, 4\n"
    "    9: SET_ALIAS  a1, r6\n"
    "   10: LABEL      L2\n"
    "   11: GET_ALIAS  r7, a3\n"
    "   12: STORE_I32  268(r1), r7\n"
    "   13: GET_ALIAS  r8, a1\n"
    "   14: STORE_I32  956(r1), r8\n"
    "   15: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,2] --> 1\n"
    "Block    1: 0 --> [3,9] --> 2\n"
    "Block    2: 1 --> [10,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
