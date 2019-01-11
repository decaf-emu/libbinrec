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
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ORI        r4, r3, 2\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: FGETSTATE  r5\n"
    "    6: ANDI       r6, r4, 3\n"
    "    7: GOTO_IF_Z  r6, L1\n"
    "    8: SLTUI      r7, r6, 2\n"
    "    9: GOTO_IF_NZ r7, L2\n"
    "   10: SEQI       r8, r6, 2\n"
    "   11: GOTO_IF_NZ r8, L3\n"
    "   12: FSETROUND  r9, r5, FLOOR\n"
    "   13: FSETSTATE  r9\n"
    "   14: GOTO       L4\n"
    "   15: LABEL      L3\n"
    "   16: FSETROUND  r10, r5, CEIL\n"
    "   17: FSETSTATE  r10\n"
    "   18: GOTO       L4\n"
    "   19: LABEL      L2\n"
    "   20: FSETROUND  r11, r5, TRUNC\n"
    "   21: FSETSTATE  r11\n"
    "   22: GOTO       L4\n"
    "   23: LABEL      L1\n"
    "   24: FSETROUND  r12, r5, NEAREST\n"
    "   25: FSETSTATE  r12\n"
    "   26: LABEL      L4\n"
    "   27: LOAD_IMM   r13, 4\n"
    "   28: SET_ALIAS  a1, r13\n"
    "   29: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,6\n"
    "Block 1: 0 --> [8,9] --> 2,5\n"
    "Block 2: 1 --> [10,11] --> 3,4\n"
    "Block 3: 2 --> [12,14] --> 7\n"
    "Block 4: 2 --> [15,18] --> 7\n"
    "Block 5: 1 --> [19,22] --> 7\n"
    "Block 6: 0 --> [23,25] --> 7\n"
    "Block 7: 6,3,4,5 --> [26,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
