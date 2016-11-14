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
    0x60,0x00,0x00,0x00,  // nop
    0x40,0x42,0x00,0x08,  // bc 2,2,0xC
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
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ADDI       r4, r3, -1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GOTO_IF_NZ r4, L3\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ANDI       r6, r5, 536870912\n"
    "    8: GOTO_IF_Z  r6, L2\n"
    "    9: LABEL      L3\n"
    "   10: LOAD_IMM   r7, 8\n"
    "   11: SET_ALIAS  a1, r7\n"
    "   12: LABEL      L1\n"
    "   13: GOTO       L1\n"
    "   14: LOAD_IMM   r8, 12\n"
    "   15: SET_ALIAS  a1, r8\n"
    "   16: LABEL      L2\n"
    "   17: LOAD_IMM   r9, 16\n"
    "   18: SET_ALIAS  a1, r9\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,2\n"
    "Block 1: 0 --> [6,8] --> 2,5\n"
    "Block 2: 1,0 --> [9,11] --> 3\n"
    "Block 3: 2,3 --> [12,13] --> 3\n"
    "Block 4: <none> --> [14,15] --> 5\n"
    "Block 5: 4,1 --> [16,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
