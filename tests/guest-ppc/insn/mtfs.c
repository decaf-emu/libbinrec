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
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: ANDI       r8, r7, -1611134977\n"
    "    9: ANDI       r9, r8, 33031936\n"
    "   10: SGTUI      r10, r9, 0\n"
    "   11: SLLI       r11, r10, 29\n"
    "   12: OR         r12, r8, r11\n"
    "   13: SRLI       r13, r12, 25\n"
    "   14: SRLI       r14, r12, 3\n"
    "   15: AND        r15, r13, r14\n"
    "   16: ANDI       r16, r15, 31\n"
    "   17: SGTUI      r17, r16, 0\n"
    "   18: SLLI       r18, r17, 30\n"
    "   19: OR         r19, r12, r18\n"
    "   20: SET_ALIAS  a3, r19\n"
    "   21: ANDI       r20, r19, 3\n"
    "   22: GOTO_IF_Z  r20, L1\n"
    "   23: SLTUI      r21, r20, 2\n"
    "   24: GOTO_IF_NZ r21, L2\n"
    "   25: SEQI       r22, r20, 2\n"
    "   26: GOTO_IF_NZ r22, L3\n"
    "   27: FSETROUND  FLOOR\n"
    "   28: GOTO       L4\n"
    "   29: LABEL      L3\n"
    "   30: FSETROUND  CEIL\n"
    "   31: GOTO       L4\n"
    "   32: LABEL      L2\n"
    "   33: FSETROUND  TRUNC\n"
    "   34: GOTO       L4\n"
    "   35: LABEL      L1\n"
    "   36: FSETROUND  NEAREST\n"
    "   37: LABEL      L4\n"
    "   38: BFEXT      r23, r7, 12, 7\n"
    "   39: SET_ALIAS  a4, r23\n"
    "   40: LOAD_IMM   r24, 4\n"
    "   41: SET_ALIAS  a1, r24\n"
    "   42: GET_ALIAS  r25, a3\n"
    "   43: GET_ALIAS  r26, a4\n"
    "   44: ANDI       r27, r25, -522241\n"
    "   45: SLLI       r28, r26, 12\n"
    "   46: OR         r29, r27, r28\n"
    "   47: SET_ALIAS  a3, r29\n"
    "   48: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,22] --> 1,6\n"
    "Block 1: 0 --> [23,24] --> 2,5\n"
    "Block 2: 1 --> [25,26] --> 3,4\n"
    "Block 3: 2 --> [27,28] --> 7\n"
    "Block 4: 2 --> [29,31] --> 7\n"
    "Block 5: 1 --> [32,34] --> 7\n"
    "Block 6: 0 --> [35,36] --> 7\n"
    "Block 7: 6,3,4,5 --> [37,48] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
