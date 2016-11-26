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
    0x50,0x83,0x29,0x8F,  // rlwimi. r3,r4,5,6,7
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: RORI       r5, r3, 19\n"
    "    5: BFINS      r6, r4, r5, 24, 2\n"
    "    6: SET_ALIAS  a2, r6\n"
    "    7: SLTSI      r7, r6, 0\n"
    "    8: SGTSI      r8, r6, 0\n"
    "    9: SEQI       r9, r6, 0\n"
    "   10: GET_ALIAS  r10, a5\n"
    "   11: BFEXT      r11, r10, 31, 1\n"
    "   12: GET_ALIAS  r12, a4\n"
    "   13: SLLI       r13, r7, 3\n"
    "   14: SLLI       r14, r8, 2\n"
    "   15: SLLI       r15, r9, 1\n"
    "   16: OR         r16, r13, r14\n"
    "   17: OR         r17, r15, r11\n"
    "   18: OR         r18, r16, r17\n"
    "   19: BFINS      r19, r12, r18, 28, 4\n"
    "   20: SET_ALIAS  a4, r19\n"
    "   21: LOAD_IMM   r20, 4\n"
    "   22: SET_ALIAS  a1, r20\n"
    "   23: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
