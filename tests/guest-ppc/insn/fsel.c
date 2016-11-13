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
    0xFC,0x22,0x20,0xEE,  // fsel %f1,%f2,%f3,%f4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: FGETSTATE  r3\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: LOAD_IMM   r5, 0.0\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: FCMP       r8, r4, r5, GE\n"
    "    8: SELECT     r9, r6, r7, r8\n"
    "    9: FSETSTATE  r3\n"
    "   10: SET_ALIAS  a2, r9\n"
    "   11: LOAD_IMM   r10, 4\n"
    "   12: SET_ALIAS  a1, r10\n"
    "   13: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
