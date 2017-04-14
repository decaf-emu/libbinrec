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
    0x7C,0x6C,0x42,0xE6,  // mftb r3
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
    "    5: ZCAST      r5, r4\n"
    "    6: SET_ALIAS  a2, r5\n"
    "    7: GOTO       L2\n"
    "    8: LABEL      L1\n"
    "    9: LOAD_IMM   r6, 0\n"
    "   10: SET_ALIAS  a2, r6\n"
    "   11: LABEL      L2\n"
    "   12: LOAD_IMM   r7, 4\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1,2\n"
    "Block 1: 0 --> [4,7] --> 3\n"
    "Block 2: 0 --> [8,10] --> 3\n"
    "Block 3: 2,1 --> [11,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
