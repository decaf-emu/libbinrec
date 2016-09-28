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
    0x7C,0x64,0x04,0xD0,  // nego r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: NOT        r3, r2\n"
    "    4: ADDI       r4, r3, 1\n"
    "    5: SRLI       r5, r3, 31\n"
    "    6: SRLI       r6, r4, 31\n"
    "    7: XORI       r7, r5, 1\n"
    "    8: AND        r8, r7, r6\n"
    "    9: GET_ALIAS  r9, a4\n"
    "   10: ANDI       r10, r9, -1073741825\n"
    "   11: LOAD_IMM   r11, 0xC0000000\n"
    "   12: SELECT     r12, r11, r8, r8\n"
    "   13: OR         r13, r10, r12\n"
    "   14: SET_ALIAS  a2, r4\n"
    "   15: SET_ALIAS  a4, r13\n"
    "   16: LOAD_IMM   r14, 4\n"
    "   17: SET_ALIAS  a1, r14\n"
    "   18: LABEL      L2\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,17] --> 2\n"
    "Block 2: 1 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
