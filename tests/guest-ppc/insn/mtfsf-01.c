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
    "   13: FGETSTATE  r12\n"
    "   14: ANDI       r13, r11, 3\n"
    "   15: GOTO_IF_Z  r13, L1\n"
    "   16: SLTUI      r14, r13, 2\n"
    "   17: GOTO_IF_NZ r14, L2\n"
    "   18: SEQI       r15, r13, 2\n"
    "   19: GOTO_IF_NZ r15, L3\n"
    "   20: FSETROUND  r16, r12, FLOOR\n"
    "   21: FSETSTATE  r16\n"
    "   22: GOTO       L4\n"
    "   23: LABEL      L3\n"
    "   24: FSETROUND  r17, r12, CEIL\n"
    "   25: FSETSTATE  r17\n"
    "   26: GOTO       L4\n"
    "   27: LABEL      L2\n"
    "   28: FSETROUND  r18, r12, TRUNC\n"
    "   29: FSETSTATE  r18\n"
    "   30: GOTO       L4\n"
    "   31: LABEL      L1\n"
    "   32: FSETROUND  r19, r12, NEAREST\n"
    "   33: FSETSTATE  r19\n"
    "   34: LABEL      L4\n"
    "   35: LOAD_IMM   r20, 4\n"
    "   36: SET_ALIAS  a1, r20\n"
    "   37: GET_ALIAS  r21, a3\n"
    "   38: GET_ALIAS  r22, a4\n"
    "   39: ANDI       r23, r21, -1611134977\n"
    "   40: SLLI       r24, r22, 12\n"
    "   41: OR         r25, r23, r24\n"
    "   42: SET_ALIAS  a3, r25\n"
    "   43: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,15] --> 1,6\n"
    "Block 1: 0 --> [16,17] --> 2,5\n"
    "Block 2: 1 --> [18,19] --> 3,4\n"
    "Block 3: 2 --> [20,22] --> 7\n"
    "Block 4: 2 --> [23,26] --> 7\n"
    "Block 5: 1 --> [27,30] --> 7\n"
    "Block 6: 0 --> [31,33] --> 7\n"
    "Block 7: 6,3,4,5 --> [34,43] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
