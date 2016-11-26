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
    0xFF,0x8C,0x00,0x80,  // mcrfs cr7,cr3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: BFEXT      r5, r3, 19, 1\n"
    "    5: BFEXT      r6, r4, 6, 1\n"
    "    6: BFEXT      r7, r4, 5, 1\n"
    "    7: BFEXT      r8, r4, 4, 1\n"
    "    8: GET_ALIAS  r9, a2\n"
    "    9: SLLI       r10, r5, 3\n"
    "   10: SLLI       r11, r6, 2\n"
    "   11: SLLI       r12, r7, 1\n"
    "   12: OR         r13, r10, r11\n"
    "   13: OR         r14, r12, r8\n"
    "   14: OR         r15, r13, r14\n"
    "   15: BFINS      r16, r9, r15, 0, 4\n"
    "   16: SET_ALIAS  a2, r16\n"
    "   17: ANDI       r17, r3, -524289\n"
    "   18: SET_ALIAS  a3, r17\n"
    "   19: LOAD_IMM   r18, 4\n"
    "   20: SET_ALIAS  a1, r18\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
