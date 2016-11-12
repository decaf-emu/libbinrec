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
    0x4A,0x00,0x10,0x03,  // bal 0xFE001000
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 0xFE001000\n"
    "    3: LOAD_IMM   r4, 8\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: GOTO       L1\n"
    "    7: LOAD_IMM   r5, 8\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L1\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 2\n"
    "Block 1: <none> --> [7,8] --> 2\n"
    "Block 2: 1,0 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
