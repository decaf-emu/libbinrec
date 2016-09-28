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
    0x7C,0x64,0x2E,0x14,  // addo r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: ADD        r4, r2, r3\n"
    "    5: SRLI       r5, r2, 31\n"
    "    6: SRLI       r6, r3, 31\n"
    "    7: XOR        r7, r5, r6\n"
    "    8: SRLI       r8, r4, 31\n"
    "    9: XORI       r9, r7, 1\n"
    "   10: XOR        r10, r5, r8\n"
    "   11: AND        r11, r9, r10\n"
    "   12: GET_ALIAS  r12, a5\n"
    "   13: ANDI       r13, r12, -1073741825\n"
    "   14: LOAD_IMM   r14, 0xC0000000\n"
    "   15: SELECT     r15, r14, r11, r11\n"
    "   16: OR         r16, r13, r15\n"
    "   17: SET_ALIAS  a2, r4\n"
    "   18: SET_ALIAS  a5, r16\n"
    "   19: LOAD_IMM   r17, 4\n"
    "   20: SET_ALIAS  a1, r17\n"
    "   21: LABEL      L2\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,20] --> 2\n"
    "Block 2: 1 --> [21,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
