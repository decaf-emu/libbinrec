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
    0x7C,0x64,0x2D,0x14,  // addeo r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: ADD        r4, r2, r3\n"
    "    5: GET_ALIAS  r5, a5\n"
    "    6: BFEXT      r6, r5, 29, 1\n"
    "    7: ADD        r7, r4, r6\n"
    "    8: SRLI       r8, r2, 31\n"
    "    9: SRLI       r9, r3, 31\n"
    "   10: XOR        r10, r8, r9\n"
    "   11: SRLI       r11, r7, 31\n"
    "   12: XORI       r12, r11, 1\n"
    "   13: AND        r13, r8, r9\n"
    "   14: AND        r14, r10, r12\n"
    "   15: OR         r15, r13, r14\n"
    "   16: BFINS      r16, r5, r15, 29, 1\n"
    "   17: XORI       r17, r10, 1\n"
    "   18: XOR        r18, r8, r11\n"
    "   19: AND        r19, r17, r18\n"
    "   20: ANDI       r20, r16, -1073741825\n"
    "   21: LOAD_IMM   r21, 0xC0000000\n"
    "   22: SELECT     r22, r21, r19, r19\n"
    "   23: OR         r23, r20, r22\n"
    "   24: SET_ALIAS  a2, r7\n"
    "   25: SET_ALIAS  a5, r23\n"
    "   26: LOAD_IMM   r24, 4\n"
    "   27: SET_ALIAS  a1, r24\n"
    "   28: LABEL      L2\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,27] --> 2\n"
    "Block 2: 1 --> [28,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
