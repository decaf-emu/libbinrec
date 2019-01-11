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
    0x7C,0x23,0x27,0xAE,  // stfiwx f1,r3,r4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: ZCAST      r6, r5\n"
    "    6: ADD        r7, r2, r6\n"
    "    7: GET_ALIAS  r8, a4\n"
    "    8: BITCAST    r9, r8\n"
    "    9: ZCAST      r10, r9\n"
    "   10: STORE_BR   0(r7), r10\n"
    "   11: LOAD_IMM   r11, 4\n"
    "   12: SET_ALIAS  a1, r11\n"
    "   13: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64 @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
