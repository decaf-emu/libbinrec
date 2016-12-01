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
    "    6: LOAD       r5, 1000(r1)\n"
    "    7: GOTO_IF_Z  r5, L2\n"
    "    8: LOAD_IMM   r6, 12\n"
    "    9: SET_ALIAS  a1, r6\n"
    "   10: GOTO       L4\n"
    "   11: LABEL      L3\n"
    "   12: LOAD_IMM   r7, 8\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: LABEL      L1\n"
    "   15: LOAD       r8, 1000(r1)\n"
    "   16: GOTO_IF_Z  r8, L1\n"
    "   17: LOAD_IMM   r9, 8\n"
    "   18: SET_ALIAS  a1, r9\n"
    "   19: GOTO       L4\n"
    "   20: LOAD_IMM   r10, 12\n"
    "   21: SET_ALIAS  a1, r10\n"
    "   22: LABEL      L2\n"
    "   23: LOAD_IMM   r11, 16\n"
    "   24: SET_ALIAS  a1, r11\n"
    "   25: LABEL      L4\n"
    "   26: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,3\n"
    "Block 1: 0 --> [6,7] --> 2,7\n"
    "Block 2: 1 --> [8,10] --> 8\n"
    "Block 3: 0 --> [11,13] --> 4\n"
    "Block 4: 3,4 --> [14,16] --> 5,4\n"
    "Block 5: 4 --> [17,19] --> 8\n"
    "Block 6: <none> --> [20,21] --> 7\n"
    "Block 7: 6,1 --> [22,24] --> 8\n"
    "Block 8: 7,2,5 --> [25,26] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
