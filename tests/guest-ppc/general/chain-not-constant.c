/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define CHAINING

static const uint8_t input[] = {
    0x4E,0x80,0x00,0x20,  // blr
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, -4\n"
    "    4: SET_ALIAS  a1, r4\n"
    /* The target address is not constant, so chaining is not used. */
    "    5: GOTO       L1\n"
    "    6: LOAD_IMM   r5, 4\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: LABEL      L1\n"
    "    9: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 2\n"
    "Block 1: <none> --> [6,7] --> 2\n"
    "Block 2: 1,0 --> [8,9] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
