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
    0xFF,0x80,0xD1,0x0C,  // mtfsfi 7,13
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
    "    4: ORI        r5, r4, 13\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: FGETSTATE  r6\n"
    "    7: FSETROUND  r7, r6, TRUNC\n"
    "    8: FSETSTATE  r7\n"
    "    9: LOAD_IMM   r8, 4\n"
    "   10: SET_ALIAS  a1, r8\n"
    "   11: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
