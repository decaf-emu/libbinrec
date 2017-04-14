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
    0x60,0x00,0x00,0x00,  // nop
    0x41,0x82,0x00,0x08,  // beq 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, 536870912\n"
    "    4: GOTO_IF_Z  r4, L3\n"
    "    5: LOAD       r5, 1008(r1)\n"
    "    6: GOTO_IF_Z  r5, L2\n"
    "    7: LOAD_IMM   r6, 12\n"
    "    8: SET_ALIAS  a1, r6\n"
    "    9: GOTO       L4\n"
    "   10: LABEL      L3\n"
    "   11: LOAD_IMM   r7, 8\n"
    "   12: SET_ALIAS  a1, r7\n"
    "   13: LABEL      L1\n"
    "   14: LOAD       r8, 1008(r1)\n"
    "   15: GOTO_IF_Z  r8, L1\n"
    "   16: LOAD_IMM   r9, 8\n"
    "   17: SET_ALIAS  a1, r9\n"
    "   18: GOTO       L4\n"
    "   19: LOAD_IMM   r10, 12\n"
    "   20: SET_ALIAS  a1, r10\n"
    "   21: LABEL      L2\n"
    "   22: LOAD_IMM   r11, 16\n"
    "   23: SET_ALIAS  a1, r11\n"
    "   24: LABEL      L4\n"
    "   25: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1,3\n"
    "Block 1: 0 --> [5,6] --> 2,7\n"
    "Block 2: 1 --> [7,9] --> 8\n"
    "Block 3: 0 --> [10,12] --> 4\n"
    "Block 4: 3,4 --> [13,15] --> 5,4\n"
    "Block 5: 4 --> [16,18] --> 8\n"
    "Block 6: <none> --> [19,20] --> 7\n"
    "Block 7: 6,1 --> [21,23] --> 8\n"
    "Block 8: 7,2,5 --> [24,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
