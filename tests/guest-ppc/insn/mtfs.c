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
    0xFD,0xFE,0x0D,0x8E,  // mtfs f1 (mtfsf 255,f1)
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BITCAST    r4, r3\n"
    "    4: ZCAST      r5, r4\n"
    "    5: ANDI       r6, r5, -1611134977\n"
    "    6: SET_ALIAS  a3, r6\n"
    "    7: ANDI       r7, r6, 3\n"
    "    8: GOTO_IF_Z  r7, L1\n"
    "    9: SLTUI      r8, r7, 2\n"
    "   10: GOTO_IF_NZ r8, L2\n"
    "   11: SEQI       r9, r7, 2\n"
    "   12: GOTO_IF_NZ r9, L3\n"
    "   13: FSETROUND  FLOOR\n"
    "   14: GOTO       L4\n"
    "   15: LABEL      L3\n"
    "   16: FSETROUND  CEIL\n"
    "   17: GOTO       L4\n"
    "   18: LABEL      L2\n"
    "   19: FSETROUND  TRUNC\n"
    "   20: GOTO       L4\n"
    "   21: LABEL      L1\n"
    "   22: FSETROUND  NEAREST\n"
    "   23: LABEL      L4\n"
    "   24: BFEXT      r10, r5, 12, 7\n"
    "   25: SET_ALIAS  a4, r10\n"
    "   26: LOAD_IMM   r11, 4\n"
    "   27: SET_ALIAS  a1, r11\n"
    "   28: GET_ALIAS  r12, a3\n"
    "   29: GET_ALIAS  r13, a4\n"
    "   30: ANDI       r14, r12, -1611134977\n"
    "   31: SLLI       r15, r13, 12\n"
    "   32: OR         r16, r14, r15\n"
    "   33: SET_ALIAS  a3, r16\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,6\n"
    "Block 1: 0 --> [9,10] --> 2,5\n"
    "Block 2: 1 --> [11,12] --> 3,4\n"
    "Block 3: 2 --> [13,14] --> 7\n"
    "Block 4: 2 --> [15,17] --> 7\n"
    "Block 5: 1 --> [18,20] --> 7\n"
    "Block 6: 0 --> [21,22] --> 7\n"
    "Block 7: 6,3,4,5 --> [23,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
