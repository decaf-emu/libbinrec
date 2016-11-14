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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: NOT        r4, r3\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: ADD        r6, r4, r5\n"
    "    6: ADDI       r7, r6, 1\n"
    "    7: SET_ALIAS  a2, r7\n"
    "    8: SRLI       r8, r4, 31\n"
    "    9: SRLI       r9, r5, 31\n"
    "   10: XOR        r10, r8, r9\n"
    "   11: SRLI       r11, r7, 31\n"
    "   12: XORI       r12, r10, 1\n"
    "   13: XOR        r13, r8, r11\n"
    "   14: AND        r14, r12, r13\n"
    "   15: GET_ALIAS  r15, a5\n"
    "   16: ANDI       r16, r15, -1073741825\n"
    "   17: LOAD_IMM   r17, 0xC0000000\n"
    "   18: SELECT     r18, r17, r14, r14\n"
    "   19: OR         r19, r16, r18\n"
    "   20: SET_ALIAS  a5, r19\n"
    "   21: LOAD_IMM   r20, 4\n"
    "   22: SET_ALIAS  a1, r20\n"
    "   23: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
