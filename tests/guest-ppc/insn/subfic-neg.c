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
    0x20, 0x60, 0xAB, 0xCD,  // subfic r3,r0,-21555
};

static const bool expected_success = true;

static const char expected[] =
    "[warning] Scanning terminated at 0x4 due to code range limit\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD       r2, 256(r1)\n"
    "    2: SET_ALIAS  a2, r2\n"
    "    3: LOAD       r3, 940(r1)\n"
    "    4: SET_ALIAS  a4, r3\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r4, a2\n"
    "    7: LOAD_IMM   r5, -21555\n"
    "    8: SUB        r6, r5, r4\n"
    "    9: SET_ALIAS  a3, r6\n"
    "   10: GET_ALIAS  r7, a4\n"
    "   11: SGTU       r8, r5, r6\n"
    "   12: BFINS      r9, r7, r8, 29, 1\n"
    "   13: SET_ALIAS  a4, r9\n"
    "   14: LOAD_IMM   r10, 4\n"
    "   15: SET_ALIAS  a1, r10\n"
    "   16: LABEL      L2\n"
    "   17: GET_ALIAS  r11, a3\n"
    "   18: STORE      268(r1), r11\n"
    "   19: GET_ALIAS  r12, a4\n"
    "   20: STORE      940(r1), r12\n"
    "   21: GET_ALIAS  r13, a1\n"
    "   22: STORE      956(r1), r13\n"
    "   23: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,4] --> 1\n"
    "Block    1: 0 --> [5,15] --> 2\n"
    "Block    2: 1 --> [16,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
