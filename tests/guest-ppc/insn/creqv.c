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
    0x4F,0xD5,0x62,0x42,  // creqv 30,21,12
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a5\n"
    "    4: BFEXT      r4, r3, 10, 1\n"
    "    5: SET_ALIAS  a3, r4\n"
    "    6: BFEXT      r5, r3, 19, 1\n"
    "    7: SET_ALIAS  a2, r5\n"
    "    8: XORI       r6, r5, 1\n"
    "    9: XOR        r7, r4, r6\n"
    "   10: SET_ALIAS  a4, r7\n"
    "   11: LOAD_IMM   r8, 4\n"
    "   12: SET_ALIAS  a1, r8\n"
    "   13: LABEL      L2\n"
    "   14: GET_ALIAS  r9, a5\n"
    "   15: ANDI       r10, r9, -3\n"
    "   16: GET_ALIAS  r11, a4\n"
    "   17: SLLI       r12, r11, 1\n"
    "   18: OR         r13, r10, r12\n"
    "   19: SET_ALIAS  a5, r13\n"
    "   20: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,12] --> 2\n"
    "Block 2: 1 --> [13,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
