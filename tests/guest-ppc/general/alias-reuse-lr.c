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
    0x7C,0x68,0x03,0xA6,  // mtlr r3
    0x4E,0x80,0x00,0x20,  // blr
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: ANDI       r3, r2, -4\n"
    "    4: SET_ALIAS  a3, r2\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: GOTO       L2\n"
    "    7: SET_ALIAS  a3, r2\n"
    "    8: LOAD_IMM   r4, 8\n"
    "    9: SET_ALIAS  a1, r4\n"
    "   10: LABEL      L2\n"
    "   11: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,6] --> 3\n"
    "Block 2: <none> --> [7,9] --> 3\n"
    "Block 3: 2,1 --> [10,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
