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
    0x48,0x00,0x00,0x08,  // b 0x8
    0x00,0x00,0x00,0x00,  // (invalid)
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GOTO       L1\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LABEL      L1\n"
    "    6: LOAD_IMM   r4, 12\n"
    "    7: SET_ALIAS  a1, r4\n"
    "    8: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,2] --> 2\n"
    "Block 1: <none> --> [3,4] --> 2\n"
    "Block 2: 1,0 --> [5,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
