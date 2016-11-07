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
    0xFF,0xC0,0x00,0x4C,  // mtfsb1 30
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ORI        r4, r3, 2\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: ANDI       r5, r4, 3\n"
    "    6: GOTO_IF_Z  r5, L1\n"
    "    7: SLTUI      r6, r5, 2\n"
    "    8: GOTO_IF_NZ r6, L2\n"
    "    9: SEQI       r7, r5, 2\n"
    "   10: GOTO_IF_NZ r7, L3\n"
    "   11: FSETROUND  FLOOR\n"
    "   12: GOTO       L4\n"
    "   13: LABEL      L3\n"
    "   14: FSETROUND  CEIL\n"
    "   15: GOTO       L4\n"
    "   16: LABEL      L2\n"
    "   17: FSETROUND  TRUNC\n"
    "   18: GOTO       L4\n"
    "   19: LABEL      L1\n"
    "   20: FSETROUND  NEAREST\n"
    "   21: LABEL      L4\n"
    "   22: LOAD_IMM   r8, 4\n"
    "   23: SET_ALIAS  a1, r8\n"
    "   24: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,6\n"
    "Block 1: 0 --> [7,8] --> 2,5\n"
    "Block 2: 1 --> [9,10] --> 3,4\n"
    "Block 3: 2 --> [11,12] --> 7\n"
    "Block 4: 2 --> [13,15] --> 7\n"
    "Block 5: 1 --> [16,18] --> 7\n"
    "Block 6: 0 --> [19,20] --> 7\n"
    "Block 7: 6,3,4,5 --> [21,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
