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
    0x0C,0x63,0x12,0x34,  // twi 3,r3,4660
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 4660\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: SEQ        r5, r4, r3\n"
    "    5: GOTO_IF_NZ r5, L1\n"
    "    6: LOAD_IMM   r6, 0\n"
    "    7: SET_ALIAS  a1, r6\n"
    "    8: LOAD       r7, 992(r1)\n"
    "    9: CALL       @r7, r1\n"
    "   10: RETURN\n"
    "   11: LABEL      L1\n"
    "   12: LOAD_IMM   r8, 4\n"
    "   13: SET_ALIAS  a1, r8\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,2\n"
    "Block 1: 0 --> [6,10] --> <none>\n"
    "Block 2: 0 --> [11,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
