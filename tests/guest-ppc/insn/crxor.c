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
    0x4F,0xD5,0x61,0x82,  // crxor 30,21,12
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 10, 1\n"
    "    4: BFEXT      r5, r3, 19, 1\n"
    "    5: XOR        r6, r4, r5\n"
    "    6: BFINS      r7, r3, r6, 1, 1\n"
    "    7: SET_ALIAS  a2, r7\n"
    "    8: LOAD_IMM   r8, 4\n"
    "    9: SET_ALIAS  a1, r8\n"
    "   10: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
