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

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a5\n"
    "    3: BFEXT      r4, r3, 10, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 19, 1\n"
    "    6: SET_ALIAS  a2, r5\n"
    "    7: AND        r6, r4, r5\n"
    "    8: SET_ALIAS  a4, r6\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: GET_ALIAS  r8, a5\n"
    "   12: ANDI       r9, r8, -3\n"
    "   13: GET_ALIAS  r10, a4\n"
    "   14: SLLI       r11, r10, 1\n"
    "   15: OR         r12, r9, r11\n"
    "   16: SET_ALIAS  a5, r12\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
