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
    0xFF,0x80,0x01,0x0C,  // mtfsfi 7,0
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, -16\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: FGETSTATE  r5\n"
    "    6: FSETROUND  r6, r5, NEAREST\n"
    "    7: FSETSTATE  r6\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
