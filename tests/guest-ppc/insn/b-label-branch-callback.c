/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define BRANCH_EXIT_TEST

static const uint8_t input[] = {
    0x48,0x00,0x00,0x08,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD       r3, 1000(r1)\n"
    "    3: GOTO_IF_Z  r3, L1\n"
    "    4: LOAD_IMM   r4, 8\n"
    "    5: SET_ALIAS  a1, r4\n"
    "    6: GOTO       L2\n"
    "    7: LOAD_IMM   r5, 4\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LOAD_IMM   r6, 8\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: LABEL      L1\n"
    "   12: LOAD_IMM   r7, 12\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: LABEL      L2\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1,3\n"
    "Block 1: 0 --> [4,6] --> 4\n"
    "Block 2: <none> --> [7,10] --> 3\n"
    "Block 3: 2,0 --> [11,13] --> 4\n"
    "Block 4: 3,1 --> [14,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
