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
    0x40,0x82,0x00,0x08,  // bne 0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ANDI       r6, r5, 2\n"
    "    8: GOTO_IF_NZ r6, L5\n"
    "    9: LOAD_IMM   r7, 12\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: LOAD       r8, 992(r1)\n"
    "   12: LOAD_IMM   r9, 4\n"
    "   13: CALL       r10, @r8, r1, r9\n"
    "   14: GOTO_IF_Z  r10, L4\n"
    "   15: GOTO       L3\n"
    "   16: LABEL      L5\n"
    "   17: LOAD_IMM   r11, 8\n"
    "   18: SET_ALIAS  a1, r11\n"
    "   19: LABEL      L2\n"
    "   20: LOAD_IMM   r12, 8\n"
    "   21: SET_ALIAS  a1, r12\n"
    "   22: LOAD       r13, 992(r1)\n"
    "   23: LOAD_IMM   r14, 8\n"
    "   24: CALL       r15, @r13, r1, r14\n"
    "   25: GOTO_IF_Z  r15, L4\n"
    "   26: GOTO       L2\n"
    "   27: LOAD_IMM   r16, 12\n"
    "   28: SET_ALIAS  a1, r16\n"
    "   29: LABEL      L3\n"
    "   30: LOAD_IMM   r17, 16\n"
    "   31: SET_ALIAS  a1, r17\n"
    "   32: LABEL      L4\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,8] --> 2,4\n"
    "Block 2: 1 --> [9,14] --> 3,9\n"
    "Block 3: 2 --> [15,15] --> 8\n"
    "Block 4: 1 --> [16,18] --> 5\n"
    "Block 5: 4,6 --> [19,25] --> 6,9\n"
    "Block 6: 5 --> [26,26] --> 5\n"
    "Block 7: <none> --> [27,28] --> 8\n"
    "Block 8: 7,3 --> [29,31] --> 9\n"
    "Block 9: 8,2,5 --> [32,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
