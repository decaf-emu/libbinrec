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
    0x7C,0x64,0x28,0x14,  // addc r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: SRLI       r6, r3, 31\n"
    "    6: SRLI       r7, r4, 31\n"
    "    7: XOR        r8, r6, r7\n"
    "    8: SRLI       r9, r5, 31\n"
    "    9: XORI       r10, r9, 1\n"
    "   10: AND        r11, r6, r7\n"
    "   11: AND        r12, r8, r10\n"
    "   12: OR         r13, r11, r12\n"
    "   13: GET_ALIAS  r14, a5\n"
    "   14: BFINS      r15, r14, r13, 29, 1\n"
    "   15: SET_ALIAS  a2, r5\n"
    "   16: SET_ALIAS  a5, r15\n"
    "   17: LOAD_IMM   r16, 4\n"
    "   18: SET_ALIAS  a1, r16\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
