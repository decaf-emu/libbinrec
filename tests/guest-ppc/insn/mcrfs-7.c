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
    0xFF,0x9C,0x00,0x80,  // mcrfs cr7,cr7
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: BFEXT      r5, r3, 2, 1\n"
    "    5: BFEXT      r6, r3, 1, 1\n"
    "    6: BFEXT      r7, r3, 0, 1\n"
    "    7: GET_ALIAS  r8, a2\n"
    "    8: SLLI       r9, r4, 3\n"
    "    9: SLLI       r10, r5, 2\n"
    "   10: SLLI       r11, r6, 1\n"
    "   11: OR         r12, r9, r10\n"
    "   12: OR         r13, r11, r7\n"
    "   13: OR         r14, r12, r13\n"
    "   14: BFINS      r15, r8, r14, 0, 4\n"
    "   15: SET_ALIAS  a2, r15\n"
    "   16: LOAD_IMM   r16, 4\n"
    "   17: SET_ALIAS  a1, r16\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
