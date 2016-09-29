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
    0x7C,0x64,0x05,0xD0,  // subfmeo r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: NOT        r4, r3\n"
    "    5: ADDI       r5, r4, -1\n"
    "    6: GET_ALIAS  r6, a4\n"
    "    7: BFEXT      r7, r6, 29, 1\n"
    "    8: ADD        r8, r5, r7\n"
    "    9: SRLI       r9, r4, 31\n"
    "   10: SRLI       r10, r8, 31\n"
    "   11: XORI       r11, r10, 1\n"
    "   12: OR         r12, r9, r11\n"
    "   13: BFINS      r13, r6, r12, 29, 1\n"
    "   14: XORI       r14, r10, 1\n"
    "   15: AND        r15, r9, r14\n"
    "   16: ANDI       r16, r13, -1073741825\n"
    "   17: LOAD_IMM   r17, 0xC0000000\n"
    "   18: SELECT     r18, r17, r15, r15\n"
    "   19: OR         r19, r16, r18\n"
    "   20: SET_ALIAS  a2, r8\n"
    "   21: SET_ALIAS  a4, r19\n"
    "   22: LOAD_IMM   r20, 4\n"
    "   23: SET_ALIAS  a1, r20\n"
    "   24: LABEL      L2\n"
    "   25: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,23] --> 2\n"
    "Block 2: 1 --> [24,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
