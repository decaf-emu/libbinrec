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
    0x4F,0xC0,0x01,0x82,  // crclr 30 (crxor 30,0,0)
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ANDI       r6, r5, 13\n"
    "    8: SET_ALIAS  a2, r6\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: LABEL      L2\n"
    "   12: LOAD       r8, 928(r1)\n"
    "   13: ANDI       r9, r8, -16\n"
    "   14: GET_ALIAS  r10, a2\n"
    "   15: OR         r11, r9, r10\n"
    "   16: STORE      928(r1), r11\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,10] --> 2\n"
    "Block 2: 1 --> [11,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
