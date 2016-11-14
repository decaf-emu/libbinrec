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
    0xFF,0x00,0x00,0x4C,  // mtfsb1 24
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ORI        r4, r3, 128\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LOAD_IMM   r5, 4\n"
    "    6: SET_ALIAS  a1, r5\n"
    "    7: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
