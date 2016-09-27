/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

/* Keep the blr outside the range of code to translate; the translator
 * should not optimize this case. */
static const struct {
    uint8_t input[4];
    uint8_t extra[4];
} input_struct = {
    {0x44,0x00,0x00,0x02},  // sc
    {0x4E,0x80,0x00,0x20},  // blr
};
#define input input_struct.input

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: LOAD_IMM   r2, 4\n"
    "    3: SET_ALIAS  a1, r2\n"
    "    4: LOAD       r3, 960(r1)\n"
    "    5: CALL_ADDR  @r3, r1\n"
    "    6: GOTO       L2\n"
    "    7: LOAD_IMM   r4, 4\n"
    "    8: SET_ALIAS  a1, r4\n"
    "    9: LABEL      L2\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,6] --> 3\n"
    "Block 2: <none> --> [7,8] --> 3\n"
    "Block 3: 2,1 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
