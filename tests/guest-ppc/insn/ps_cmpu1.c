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
    0x13,0x81,0x10,0x80,  // ps_cmpu1 cr7,f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: VEXTRACT   r4, r3, 1\n"
    "    4: FCAST      r5, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: VEXTRACT   r7, r6, 1\n"
    "    7: FCAST      r8, r7\n"
    "    8: FCMP       r9, r5, r8, LT\n"
    "    9: FCMP       r10, r5, r8, GT\n"
    "   10: FCMP       r11, r5, r8, EQ\n"
    "   11: FCMP       r12, r5, r8, UN\n"
    "   12: GET_ALIAS  r13, a4\n"
    "   13: SLLI       r14, r9, 3\n"
    "   14: SLLI       r15, r10, 2\n"
    "   15: SLLI       r16, r11, 1\n"
    "   16: OR         r17, r14, r15\n"
    "   17: OR         r18, r16, r12\n"
    "   18: OR         r19, r17, r18\n"
    "   19: BFINS      r20, r13, r19, 0, 4\n"
    "   20: SET_ALIAS  a4, r20\n"
    "   21: GET_ALIAS  r21, a5\n"
    "   22: BFEXT      r22, r21, 12, 7\n"
    "   23: ANDI       r23, r22, 112\n"
    "   24: SLLI       r24, r9, 3\n"
    "   25: SLLI       r25, r10, 2\n"
    "   26: SLLI       r26, r11, 1\n"
    "   27: OR         r27, r24, r25\n"
    "   28: OR         r28, r26, r12\n"
    "   29: OR         r29, r27, r28\n"
    "   30: OR         r30, r23, r29\n"
    "   31: GET_ALIAS  r31, a5\n"
    "   32: BFINS      r32, r31, r30, 12, 7\n"
    "   33: SET_ALIAS  a5, r32\n"
    "   34: FGETSTATE  r33\n"
    "   35: FTESTEXC   r34, r33, INVALID\n"
    "   36: GOTO_IF_Z  r34, L1\n"
    "   37: FCLEAREXC  r35, r33\n"
    "   38: FSETSTATE  r35\n"
    "   39: NOT        r36, r32\n"
    "   40: ORI        r37, r32, 16777216\n"
    "   41: ANDI       r38, r36, 16777216\n"
    "   42: SET_ALIAS  a5, r37\n"
    "   43: GOTO_IF_Z  r38, L2\n"
    "   44: ORI        r39, r37, -2147483648\n"
    "   45: SET_ALIAS  a5, r39\n"
    "   46: LABEL      L2\n"
    "   47: LABEL      L1\n"
    "   48: LOAD_IMM   r40, 4\n"
    "   49: SET_ALIAS  a1, r40\n"
    "   50: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,36] --> 1,4\n"
    "Block 1: 0 --> [37,43] --> 2,3\n"
    "Block 2: 1 --> [44,45] --> 3\n"
    "Block 3: 2,1 --> [46,46] --> 4\n"
    "Block 4: 3,0 --> [47,50] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
