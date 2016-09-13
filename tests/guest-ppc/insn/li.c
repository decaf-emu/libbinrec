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
    0x38, 0x60, 0x12, 0x34,  // li r3,4660
    0x38, 0x80, 0xAB, 0xCD,  // li r4,-21555
};

static const bool expected_success = true;

static const char expected[] =
    "[warning] Scanning terminated at 0x8 due to code range limit\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 4660\n"
    "    3: SET_ALIAS  a2, r2\n"
    "    4: LOAD_IMM   r3, -21555\n"
    "    5: SET_ALIAS  a3, r3\n"
    "    6: LOAD_IMM   r4, 8\n"
    "    7: SET_ALIAS  a1, r4\n"
    "    8: LABEL      L2\n"
    "    9: GET_ALIAS  r5, a2\n"
    "   10: STORE      268(r1), r5\n"
    "   11: GET_ALIAS  r6, a3\n"
    "   12: STORE      272(r1), r6\n"
    "   13: GET_ALIAS  r7, a1\n"
    "   14: STORE      956(r1), r7\n"
    "   15: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,0] --> 1\n"
    "Block    1: 0 --> [1,7] --> 2\n"
    "Block    2: 1 --> [8,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
