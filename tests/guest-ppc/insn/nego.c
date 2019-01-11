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
    0x7C,0x64,0x04,0xD0,  // nego r3,r4
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
    "    4: ADDI       r5, r4, 1\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: SRLI       r6, r4, 31\n"
    "    7: SRLI       r7, r5, 31\n"
    "    8: XORI       r8, r6, 1\n"
    "    9: AND        r9, r8, r7\n"
    "   10: GET_ALIAS  r10, a4\n"
    "   11: ANDI       r11, r10, -1073741825\n"
    "   12: LOAD_IMM   r12, 0xC0000000\n"
    "   13: SELECT     r13, r12, r9, r9\n"
    "   14: OR         r14, r11, r13\n"
    "   15: SET_ALIAS  a4, r14\n"
    "   16: LOAD_IMM   r15, 4\n"
    "   17: SET_ALIAS  a1, r15\n"
    "   18: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
