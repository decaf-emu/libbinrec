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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ADDI       r4, r3, -1\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: GOTO_IF_NZ r4, L3\n"
    "    6: LOAD_IMM   r5, 12\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: LOAD       r6, 1000(r1)\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: CALL       r8, @r6, r1, r7\n"
    "   11: GOTO_IF_Z  r8, L4\n"
    "   12: GOTO       L2\n"
    "   13: LABEL      L3\n"
    "   14: LOAD_IMM   r9, 8\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L1\n"
    "   17: LOAD_IMM   r10, 8\n"
    "   18: SET_ALIAS  a1, r10\n"
    "   19: LOAD       r11, 1000(r1)\n"
    "   20: LOAD_IMM   r12, 8\n"
    "   21: CALL       r13, @r11, r1, r12\n"
    "   22: GOTO_IF_Z  r13, L4\n"
    "   23: GOTO       L1\n"
    "   24: LOAD_IMM   r14, 12\n"
    "   25: SET_ALIAS  a1, r14\n"
    "   26: LABEL      L2\n"
    "   27: LOAD_IMM   r15, 16\n"
    "   28: SET_ALIAS  a1, r15\n"
    "   29: LABEL      L4\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,3\n"
    "Block 1: 0 --> [6,11] --> 2,8\n"
    "Block 2: 1 --> [12,12] --> 7\n"
    "Block 3: 0 --> [13,15] --> 4\n"
    "Block 4: 3,5 --> [16,22] --> 5,8\n"
    "Block 5: 4 --> [23,23] --> 4\n"
    "Block 6: <none> --> [24,25] --> 7\n"
    "Block 7: 6,2 --> [26,28] --> 8\n"
    "Block 8: 7,1,4 --> [29,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
