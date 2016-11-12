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
    0x4F,0xD5,0x62,0x02,  // crand 30,21,12
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BFEXT      r6, r5, 10, 1\n"
    "    7: BFEXT      r7, r5, 19, 1\n"
    "    8: AND        r8, r6, r7\n"
    "    9: SET_ALIAS  a3, r8\n"
    "   10: LOAD_IMM   r9, 4\n"
    "   11: SET_ALIAS  a1, r9\n"
    "   12: GET_ALIAS  r10, a2\n"
    "   13: ANDI       r11, r10, -3\n"
    "   14: GET_ALIAS  r12, a3\n"
    "   15: SLLI       r13, r12, 1\n"
    "   16: OR         r14, r11, r13\n"
    "   17: SET_ALIAS  a2, r14\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
