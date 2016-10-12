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
    0x42,0x42,0x00,0x08,  // bdz 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: ADDI       r4, r3, -1\n"
    "    5: SET_ALIAS  a2, r4\n"
    "    6: GOTO_IF_NZ r4, L5\n"
    "    7: LOAD_IMM   r5, 12\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LOAD       r6, 992(r1)\n"
    "   10: LOAD_IMM   r7, 4\n"
    "   11: CALL       r8, @r6, r1, r7\n"
    "   12: GOTO_IF_Z  r8, L4\n"
    "   13: GOTO       L3\n"
    "   14: LABEL      L5\n"
    "   15: LOAD_IMM   r9, 8\n"
    "   16: SET_ALIAS  a1, r9\n"
    "   17: LABEL      L2\n"
    "   18: LOAD_IMM   r10, 8\n"
    "   19: SET_ALIAS  a1, r10\n"
    "   20: LOAD       r11, 992(r1)\n"
    "   21: LOAD_IMM   r12, 8\n"
    "   22: CALL       r13, @r11, r1, r12\n"
    "   23: GOTO_IF_Z  r13, L4\n"
    "   24: GOTO       L2\n"
    "   25: LOAD_IMM   r14, 12\n"
    "   26: SET_ALIAS  a1, r14\n"
    "   27: LABEL      L3\n"
    "   28: LOAD_IMM   r15, 16\n"
    "   29: SET_ALIAS  a1, r15\n"
    "   30: LABEL      L4\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,6] --> 2,4\n"
    "Block 2: 1 --> [7,12] --> 3,9\n"
    "Block 3: 2 --> [13,13] --> 8\n"
    "Block 4: 1 --> [14,16] --> 5\n"
    "Block 5: 4,6 --> [17,23] --> 6,9\n"
    "Block 6: 5 --> [24,24] --> 5\n"
    "Block 7: <none> --> [25,26] --> 8\n"
    "Block 8: 7,3 --> [27,29] --> 9\n"
    "Block 9: 8,2,5 --> [30,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
