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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: BFEXT      r5, r4, 29, 1\n"
    "    5: ADD        r6, r3, r5\n"
    "    6: SET_ALIAS  a2, r6\n"
    "    7: SRLI       r7, r3, 31\n"
    "    8: SRLI       r8, r6, 31\n"
    "    9: XORI       r9, r8, 1\n"
    "   10: AND        r10, r7, r9\n"
    "   11: BFINS      r11, r4, r10, 29, 1\n"
    "   12: SET_ALIAS  a4, r11\n"
    "   13: LOAD_IMM   r12, 4\n"
    "   14: SET_ALIAS  a1, r12\n"
    "   15: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
