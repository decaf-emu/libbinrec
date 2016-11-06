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
    "   10: ANDI       r10, r8, -1073741840\n"
    "   11: OR         r11, r10, r9\n"
    "   12: SRLI       r12, r11, 25\n"
    "   13: SRLI       r13, r11, 3\n"
    "   14: AND        r14, r12, r13\n"
    "   15: ANDI       r15, r14, 31\n"
    "   16: SGTUI      r16, r15, 0\n"
    "   17: SLLI       r17, r16, 30\n"
    "   18: OR         r18, r11, r17\n"
    "   19: SET_ALIAS  a3, r18\n"
    "   20: ANDI       r19, r18, 3\n"
    "   21: GOTO_IF_Z  r19, L1\n"
    "   22: SLTUI      r20, r19, 2\n"
    "   23: GOTO_IF_NZ r20, L2\n"
    "   24: SEQI       r21, r19, 2\n"
    "   25: GOTO_IF_NZ r21, L3\n"
    "   26: FSETROUND  FLOOR\n"
    "   27: GOTO       L4\n"
    "   28: LABEL      L3\n"
    "   29: FSETROUND  CEIL\n"
    "   30: GOTO       L4\n"
    "   31: LABEL      L2\n"
    "   32: FSETROUND  TRUNC\n"
    "   33: GOTO       L4\n"
    "   34: LABEL      L1\n"
    "   35: FSETROUND  NEAREST\n"
    "   36: LABEL      L4\n"
    "   37: LOAD_IMM   r22, 4\n"
    "   38: SET_ALIAS  a1, r22\n"
    "   39: GET_ALIAS  r23, a3\n"
    "   40: GET_ALIAS  r24, a4\n"
    "   41: ANDI       r25, r23, -522241\n"
    "   42: SLLI       r26, r24, 12\n"
    "   43: OR         r27, r25, r26\n"
    "   44: SET_ALIAS  a3, r27\n"
    "   45: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,21] --> 1,6\n"
    "Block 1: 0 --> [22,23] --> 2,5\n"
    "Block 2: 1 --> [24,25] --> 3,4\n"
    "Block 3: 2 --> [26,27] --> 7\n"
    "Block 4: 2 --> [28,30] --> 7\n"
    "Block 5: 1 --> [31,33] --> 7\n"
    "Block 6: 0 --> [34,35] --> 7\n"
    "Block 7: 6,3,4,5 --> [36,45] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
