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
    0x4F,0xD5,0x63,0x42,  // crorc 30,21,12
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
    "    5: XORI       r6, r5, 1\n"
    "    6: OR         r7, r4, r6\n"
    "    7: BFINS      r8, r3, r7, 1, 1\n"
    "    8: SET_ALIAS  a2, r8\n"
    "    9: LOAD_IMM   r9, 4\n"
    "   10: SET_ALIAS  a1, r9\n"
    "   11: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
