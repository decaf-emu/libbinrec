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
    0x7C,0x64,0x2C,0x14,  // addco r3,r4,r5
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
    "    9: XORI       r9, r8, 1\n"
    "   10: AND        r10, r5, r6\n"
    "   11: AND        r11, r7, r9\n"
    "   12: OR         r12, r10, r11\n"
    "   13: GET_ALIAS  r13, a5\n"
    "   14: BFINS      r14, r13, r12, 29, 1\n"
    "   15: XORI       r15, r7, 1\n"
    "   16: XOR        r16, r5, r8\n"
    "   17: AND        r17, r15, r16\n"
    "   18: ANDI       r18, r14, -1073741825\n"
    "   19: LOAD_IMM   r19, 0xC0000000\n"
    "   20: SELECT     r20, r19, r17, r17\n"
    "   21: OR         r21, r18, r20\n"
    "   22: SET_ALIAS  a2, r4\n"
    "   23: SET_ALIAS  a5, r21\n"
    "   24: LOAD_IMM   r22, 4\n"
    "   25: SET_ALIAS  a1, r22\n"
    "   26: LABEL      L2\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,25] --> 2\n"
    "Block 2: 1 --> [26,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
