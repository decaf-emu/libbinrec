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
    0x60,0x00,0x00,0x00,  // nop
    0x41,0x82,0x00,0x08,  // beq 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: ANDI       r4, r3, 536870912\n"
    "    5: GOTO_IF_Z  r4, L5\n"
    "    6: LOAD_IMM   r5, 12\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: LOAD       r6, 992(r1)\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: CALL       r8, @r6, r1, r7\n"
    "   11: GOTO_IF_Z  r8, L4\n"
    "   12: GOTO       L3\n"
    "   13: LABEL      L5\n"
    "   14: LOAD_IMM   r9, 8\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L2\n"
    "   17: LOAD_IMM   r10, 8\n"
    "   18: SET_ALIAS  a1, r10\n"
    "   19: LOAD       r11, 992(r1)\n"
    "   20: LOAD_IMM   r12, 8\n"
    "   21: CALL       r13, @r11, r1, r12\n"
    "   22: GOTO_IF_Z  r13, L4\n"
    "   23: GOTO       L2\n"
    "   24: LOAD_IMM   r14, 12\n"
    "   25: SET_ALIAS  a1, r14\n"
    "   26: LABEL      L3\n"
    "   27: LOAD_IMM   r15, 16\n"
    "   28: SET_ALIAS  a1, r15\n"
    "   29: LABEL      L4\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,5] --> 2,4\n"
    "Block 2: 1 --> [6,11] --> 3,9\n"
    "Block 3: 2 --> [12,12] --> 8\n"
    "Block 4: 1 --> [13,15] --> 5\n"
    "Block 5: 4,6 --> [16,22] --> 6,9\n"
    "Block 6: 5 --> [23,23] --> 5\n"
    "Block 7: <none> --> [24,25] --> 8\n"
    "Block 8: 7,3 --> [26,28] --> 9\n"
    "Block 9: 8,2,5 --> [29,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
