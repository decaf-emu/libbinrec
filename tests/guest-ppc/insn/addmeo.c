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
    0x7C,0x64,0x05,0xD4,  // addmeo r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: ADDI       r4, r3, -1\n"
    "    5: GET_ALIAS  r5, a4\n"
    "    6: BFEXT      r6, r5, 29, 1\n"
    "    7: ADD        r7, r4, r6\n"
    "    8: SRLI       r8, r3, 31\n"
    "    9: SRLI       r9, r7, 31\n"
    "   10: XORI       r10, r9, 1\n"
    "   11: OR         r11, r8, r10\n"
    "   12: BFINS      r12, r5, r11, 29, 1\n"
    "   13: XORI       r13, r9, 1\n"
    "   14: AND        r14, r8, r13\n"
    "   15: ANDI       r15, r12, -1073741825\n"
    "   16: LOAD_IMM   r16, 0xC0000000\n"
    "   17: SELECT     r17, r16, r14, r14\n"
    "   18: OR         r18, r15, r17\n"
    "   19: SET_ALIAS  a2, r7\n"
    "   20: SET_ALIAS  a4, r18\n"
    "   21: LOAD_IMM   r19, 4\n"
    "   22: SET_ALIAS  a1, r19\n"
    "   23: LABEL      L2\n"
    "   24: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,22] --> 2\n"
    "Block 2: 1 --> [23,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
