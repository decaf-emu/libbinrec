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
    0x3B, 0xDF, 0xAB, 0xCD,  // addi r30,r31,-21555
};

static const bool expected_success = true;

static const char expected[] =
    "[warning] Scanning terminated at 0x4 due to code range limit\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD       r2, 380(r1)\n"
    "    2: SET_ALIAS  a3, r2\n"
    "    3: LABEL      L1\n"
    "    4: GET_ALIAS  r3, a3\n"
    "    5: ADDI       r4, r3, -21555\n"
    "    6: SET_ALIAS  a2, r4\n"
    "    7: LOAD_IMM   r5, 4\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L2\n"
    "   10: GET_ALIAS  r6, a2\n"
    "   11: STORE      376(r1), r6\n"
    "   12: GET_ALIAS  r7, a1\n"
    "   13: STORE      956(r1), r7\n"
    "   14: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,2] --> 1\n"
    "Block    1: 0 --> [3,8] --> 2\n"
    "Block    2: 1 --> [9,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
