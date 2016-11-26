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
    0x7F,0x83,0x20,0x40,  // cmpl cr7,r3,r4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: GET_ALIAS  r7, a3\n"
    "    4: SLTU       r4, r3, r7\n"
    "    5: SGTU       r5, r3, r7\n"
    "    6: SEQ        r6, r3, r7\n"
    "    7: GET_ALIAS  r8, a5\n"
    "    8: BFEXT      r9, r8, 31, 1\n"
    "    9: GET_ALIAS  r10, a4\n"
    "   10: SLLI       r11, r4, 3\n"
    "   11: SLLI       r12, r5, 2\n"
    "   12: SLLI       r13, r6, 1\n"
    "   13: OR         r14, r11, r12\n"
    "   14: OR         r15, r13, r9\n"
    "   15: OR         r16, r14, r15\n"
    "   16: BFINS      r17, r10, r16, 0, 4\n"
    "   17: SET_ALIAS  a4, r17\n"
    "   18: LOAD_IMM   r18, 4\n"
    "   19: SET_ALIAS  a1, r18\n"
    "   20: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
