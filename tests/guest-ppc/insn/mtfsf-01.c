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
    0xFC,0x02,0x0D,0x8E,  // mtfsf 1,f1
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: GET_ALIAS  r8, a3\n"
    "    9: ANDI       r9, r7, 15\n"
    "   10: ANDI       r10, r8, -16\n"
    "   11: OR         r11, r10, r9\n"
    "   12: SET_ALIAS  a3, r11\n"
    "   13: ANDI       r12, r11, 3\n"
    "   14: GOTO_IF_Z  r12, L1\n"
    "   15: SLTUI      r13, r12, 2\n"
    "   16: GOTO_IF_NZ r13, L2\n"
    "   17: SEQI       r14, r12, 2\n"
    "   18: GOTO_IF_NZ r14, L3\n"
    "   19: FSETROUND  FLOOR\n"
    "   20: GOTO       L4\n"
    "   21: LABEL      L3\n"
    "   22: FSETROUND  CEIL\n"
    "   23: GOTO       L4\n"
    "   24: LABEL      L2\n"
    "   25: FSETROUND  TRUNC\n"
    "   26: GOTO       L4\n"
    "   27: LABEL      L1\n"
    "   28: FSETROUND  NEAREST\n"
    "   29: LABEL      L4\n"
    "   30: LOAD_IMM   r15, 4\n"
    "   31: SET_ALIAS  a1, r15\n"
    "   32: GET_ALIAS  r16, a3\n"
    "   33: GET_ALIAS  r17, a4\n"
    "   34: ANDI       r18, r16, -1611134977\n"
    "   35: SLLI       r19, r17, 12\n"
    "   36: OR         r20, r18, r19\n"
    "   37: SET_ALIAS  a3, r20\n"
    "   38: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,14] --> 1,6\n"
    "Block 1: 0 --> [15,16] --> 2,5\n"
    "Block 2: 1 --> [17,18] --> 3,4\n"
    "Block 3: 2 --> [19,20] --> 7\n"
    "Block 4: 2 --> [21,23] --> 7\n"
    "Block 5: 1 --> [24,26] --> 7\n"
    "Block 6: 0 --> [27,28] --> 7\n"
    "Block 7: 6,3,4,5 --> [29,38] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
