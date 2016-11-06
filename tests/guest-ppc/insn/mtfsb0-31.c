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
    0xFF,0xE0,0x00,0x8C,  // mtfsb0 31
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: ANDI       r6, r5, -2\n"
    "    7: SET_ALIAS  a2, r6\n"
    "    8: ANDI       r7, r6, 3\n"
    "    9: GOTO_IF_Z  r7, L1\n"
    "   10: SLTUI      r8, r7, 2\n"
    "   11: GOTO_IF_NZ r8, L2\n"
    "   12: SEQI       r9, r7, 2\n"
    "   13: GOTO_IF_NZ r9, L3\n"
    "   14: FSETROUND  FLOOR\n"
    "   15: GOTO       L4\n"
    "   16: LABEL      L3\n"
    "   17: FSETROUND  CEIL\n"
    "   18: GOTO       L4\n"
    "   19: LABEL      L2\n"
    "   20: FSETROUND  TRUNC\n"
    "   21: GOTO       L4\n"
    "   22: LABEL      L1\n"
    "   23: FSETROUND  NEAREST\n"
    "   24: LABEL      L4\n"
    "   25: LOAD_IMM   r10, 4\n"
    "   26: SET_ALIAS  a1, r10\n"
    "   27: GET_ALIAS  r11, a2\n"
    "   28: GET_ALIAS  r12, a3\n"
    "   29: ANDI       r13, r11, -1611134977\n"
    "   30: SLLI       r14, r12, 12\n"
    "   31: OR         r15, r13, r14\n"
    "   32: SET_ALIAS  a2, r15\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,6\n"
    "Block 1: 0 --> [10,11] --> 2,5\n"
    "Block 2: 1 --> [12,13] --> 3,4\n"
    "Block 3: 2 --> [14,15] --> 7\n"
    "Block 4: 2 --> [16,18] --> 7\n"
    "Block 5: 1 --> [19,21] --> 7\n"
    "Block 6: 0 --> [22,23] --> 7\n"
    "Block 7: 6,3,4,5 --> [24,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
