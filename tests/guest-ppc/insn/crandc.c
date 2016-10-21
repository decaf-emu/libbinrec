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
    0x4F,0xD5,0x61,0x02,  // crandc 30,21,12
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 10, 1\n"
    "    4: BFEXT      r5, r3, 19, 1\n"
    "    5: XORI       r6, r5, 1\n"
    "    6: AND        r7, r4, r6\n"
    "    7: SET_ALIAS  a2, r7\n"
    "    8: LOAD_IMM   r8, 4\n"
    "    9: SET_ALIAS  a1, r8\n"
    "   10: GET_ALIAS  r9, a3\n"
    "   11: ANDI       r10, r9, -3\n"
    "   12: GET_ALIAS  r11, a2\n"
    "   13: SLLI       r12, r11, 1\n"
    "   14: OR         r13, r10, r12\n"
    "   15: SET_ALIAS  a3, r13\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
