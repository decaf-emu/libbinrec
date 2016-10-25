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
    0x4F,0xC0,0x02,0x42,  // crset 30 (creqv 30,0,0)
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: SET_ALIAS  a2, r3\n"
    "    4: LOAD_IMM   r4, 4\n"
    "    5: SET_ALIAS  a1, r4\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: ANDI       r6, r5, -3\n"
    "    8: GET_ALIAS  r7, a2\n"
    "    9: SLLI       r8, r7, 1\n"
    "   10: OR         r9, r6, r8\n"
    "   11: SET_ALIAS  a3, r9\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
