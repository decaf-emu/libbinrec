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
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: ADD        r5, r3, r4\n"
    "    6: SRLI       r6, r3, 31\n"
    "    7: SRLI       r7, r4, 31\n"
    "    8: XOR        r8, r6, r7\n"
    "    9: SRLI       r9, r5, 31\n"
    "   10: XORI       r10, r8, 1\n"
    "   11: XOR        r11, r6, r9\n"
    "   12: AND        r12, r10, r11\n"
    "   13: GET_ALIAS  r13, a5\n"
    "   14: ANDI       r14, r13, -1073741825\n"
    "   15: LOAD_IMM   r15, 0xC0000000\n"
    "   16: SELECT     r16, r15, r12, r12\n"
    "   17: OR         r17, r14, r16\n"
    "   18: SET_ALIAS  a2, r5\n"
    "   19: SET_ALIAS  a5, r17\n"
    "   20: LOAD_IMM   r18, 4\n"
    "   21: SET_ALIAS  a1, r18\n"
    "   22: LABEL      L2\n"
    "   23: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,21] --> 2\n"
    "Block 2: 1 --> [22,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
