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
    0x60,0x00,0x00,0x00,  // nop
    0x48,0x00,0x00,0x00,  // b 0x4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 4\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: LABEL      L1\n"
    "    5: GOTO       L1\n"
    "    6: LOAD_IMM   r4, 8\n"
    "    7: SET_ALIAS  a1, r4\n"
    "    8: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1\n"
    "Block 1: 0,1 --> [4,5] --> 1\n"
    "Block 2: <none> --> [6,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
