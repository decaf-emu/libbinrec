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
    0x7C,0x64,0x28,0x10,  // subfc r3,r4,r5
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
    "   11: XORI       r11, r10, 1\n"
    "   12: AND        r12, r7, r8\n"
    "   13: AND        r13, r9, r11\n"
    "   14: OR         r14, r12, r13\n"
    "   15: GET_ALIAS  r15, a5\n"
    "   16: BFINS      r16, r15, r14, 29, 1\n"
    "   17: SET_ALIAS  a2, r6\n"
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
