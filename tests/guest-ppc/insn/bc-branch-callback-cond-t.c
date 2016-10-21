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
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, 536870912\n"
    "    4: GOTO_IF_Z  r4, L3\n"
    "    5: LOAD_IMM   r5, 12\n"
    "    6: SET_ALIAS  a1, r5\n"
    "    7: LOAD       r6, 992(r1)\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: CALL       r8, @r6, r1, r7\n"
    "   10: GOTO_IF_Z  r8, L4\n"
    "   11: GOTO       L2\n"
    "   12: LABEL      L3\n"
    "   13: LOAD_IMM   r9, 8\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LABEL      L1\n"
    "   16: LOAD_IMM   r10, 8\n"
    "   17: SET_ALIAS  a1, r10\n"
    "   18: LOAD       r11, 992(r1)\n"
    "   19: LOAD_IMM   r12, 8\n"
    "   20: CALL       r13, @r11, r1, r12\n"
    "   21: GOTO_IF_Z  r13, L4\n"
    "   22: GOTO       L1\n"
    "   23: LOAD_IMM   r14, 12\n"
    "   24: SET_ALIAS  a1, r14\n"
    "   25: LABEL      L2\n"
    "   26: LOAD_IMM   r15, 16\n"
    "   27: SET_ALIAS  a1, r15\n"
    "   28: LABEL      L4\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1,3\n"
    "Block 1: 0 --> [5,10] --> 2,8\n"
    "Block 2: 1 --> [11,11] --> 7\n"
    "Block 3: 0 --> [12,14] --> 4\n"
    "Block 4: 3,5 --> [15,21] --> 5,8\n"
    "Block 5: 4 --> [22,22] --> 4\n"
    "Block 6: <none> --> [23,24] --> 7\n"
    "Block 7: 6,2 --> [25,27] --> 8\n"
    "Block 8: 7,1,4 --> [28,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
