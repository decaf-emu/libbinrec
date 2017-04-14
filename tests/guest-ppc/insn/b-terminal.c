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
    0x4A,0x00,0x10,0x00,  // b 0xFE001004
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 0xFE001004\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: GOTO       L1\n"
    "    5: LOAD_IMM   r4, 8\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L1\n"
    "    8: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 2\n"
    "Block 1: <none> --> [5,6] --> 2\n"
    "Block 2: 1,0 --> [7,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
