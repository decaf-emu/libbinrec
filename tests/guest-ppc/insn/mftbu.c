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
    0x7C,0x6D,0x42,0xE6,  // mftbu r3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD       r3, 976(r1)\n"
    "    3: GOTO_IF_Z  r3, L1\n"
    "    4: CALL       r4, @r3, r1\n"
    "    5: SRLI       r6, r4, 32\n"
    "    6: ZCAST      r5, r6\n"
    "    7: SET_ALIAS  a2, r5\n"
    "    8: GOTO       L2\n"
    "    9: LABEL      L1\n"
    "   10: LOAD_IMM   r7, 0\n"
    "   11: SET_ALIAS  a2, r7\n"
    "   12: LABEL      L2\n"
    "   13: LOAD_IMM   r8, 4\n"
    "   14: SET_ALIAS  a1, r8\n"
    "   15: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1,2\n"
    "Block 1: 0 --> [4,8] --> 3\n"
    "Block 2: 0 --> [9,11] --> 3\n"
    "Block 3: 2,1 --> [12,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
