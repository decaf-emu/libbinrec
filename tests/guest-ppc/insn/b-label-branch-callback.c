/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define BRANCH_CALLBACK

static const uint8_t input[] = {
    0x48,0x00,0x00,0x08,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 8\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LOAD       r4, 992(r1)\n"
    "    6: LOAD_IMM   r5, 0\n"
    "    7: CALL       r6, @r4, r1, r5\n"
    "    8: GOTO_IF_Z  r6, L4\n"
    "    9: GOTO       L3\n"
    "   10: LOAD_IMM   r7, 4\n"
    "   11: SET_ALIAS  a1, r7\n"
    "   12: LABEL      L2\n"
    "   13: LOAD_IMM   r8, 8\n"
    "   14: SET_ALIAS  a1, r8\n"
    "   15: LABEL      L3\n"
    "   16: LOAD_IMM   r9, 12\n"
    "   17: SET_ALIAS  a1, r9\n"
    "   18: LABEL      L4\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,8] --> 2,6\n"
    "Block 2: 1 --> [9,9] --> 5\n"
    "Block 3: <none> --> [10,11] --> 4\n"
    "Block 4: 3 --> [12,14] --> 5\n"
    "Block 5: 4,2 --> [15,17] --> 6\n"
    "Block 6: 5,1 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
