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
    0x7C,0x64,0x01,0x94,  // addze r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: BFEXT      r4, r3, 29, 1\n"
    "    5: ADD        r5, r2, r4\n"
    "    6: SRLI       r6, r2, 31\n"
    "    7: SRLI       r7, r5, 31\n"
    "    8: XORI       r8, r7, 1\n"
    "    9: AND        r9, r6, r8\n"
    "   10: BFINS      r10, r3, r9, 29, 1\n"
    "   11: SET_ALIAS  a2, r5\n"
    "   12: SET_ALIAS  a4, r10\n"
    "   13: LOAD_IMM   r11, 4\n"
    "   14: SET_ALIAS  a1, r11\n"
    "   15: LABEL      L2\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,14] --> 2\n"
    "Block 2: 1 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
