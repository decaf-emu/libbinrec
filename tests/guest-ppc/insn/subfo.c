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
    0x7C,0x64,0x2C,0x50,  // subfo r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: NOT        r3, r2\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: ADD        r5, r3, r4\n"
    "    6: ADDI       r6, r5, 1\n"
    "    7: SRLI       r7, r3, 31\n"
    "    8: SRLI       r8, r4, 31\n"
    "    9: XOR        r9, r7, r8\n"
    "   10: SRLI       r10, r6, 31\n"
    "   11: XORI       r11, r9, 1\n"
    "   12: XOR        r12, r7, r10\n"
    "   13: AND        r13, r11, r12\n"
    "   14: GET_ALIAS  r14, a5\n"
    "   15: ANDI       r15, r14, -1073741825\n"
    "   16: LOAD_IMM   r16, 0xC0000000\n"
    "   17: SELECT     r17, r16, r13, r13\n"
    "   18: OR         r18, r15, r17\n"
    "   19: SET_ALIAS  a2, r6\n"
    "   20: SET_ALIAS  a5, r18\n"
    "   21: LOAD_IMM   r19, 4\n"
    "   22: SET_ALIAS  a1, r19\n"
    "   23: LABEL      L2\n"
    "   24: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,22] --> 2\n"
    "Block 2: 1 --> [23,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
