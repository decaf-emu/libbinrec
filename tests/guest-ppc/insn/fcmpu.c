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
    0xFF,0x81,0x10,0x00,  // fcmpu cr7,f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: FCMP       r5, r3, r4, LT\n"
    "    5: FCMP       r6, r3, r4, GT\n"
    "    6: FCMP       r7, r3, r4, EQ\n"
    "    7: FCMP       r8, r3, r4, UN\n"
    "    8: GET_ALIAS  r9, a4\n"
    "    9: SLLI       r10, r5, 3\n"
    "   10: SLLI       r11, r6, 2\n"
    "   11: SLLI       r12, r7, 1\n"
    "   12: OR         r13, r10, r11\n"
    "   13: OR         r14, r12, r8\n"
    "   14: OR         r15, r13, r14\n"
    "   15: BFINS      r16, r9, r15, 0, 4\n"
    "   16: SET_ALIAS  a4, r16\n"
    "   17: GET_ALIAS  r17, a5\n"
    "   18: BFEXT      r18, r17, 12, 7\n"
    "   19: ANDI       r19, r18, 112\n"
    "   20: SLLI       r20, r5, 3\n"
    "   21: SLLI       r21, r6, 2\n"
    "   22: SLLI       r22, r7, 1\n"
    "   23: OR         r23, r20, r21\n"
    "   24: OR         r24, r22, r8\n"
    "   25: OR         r25, r23, r24\n"
    "   26: OR         r26, r19, r25\n"
    "   27: GET_ALIAS  r27, a5\n"
    "   28: BFINS      r28, r27, r26, 12, 7\n"
    "   29: SET_ALIAS  a5, r28\n"
    "   30: FGETSTATE  r29\n"
    "   31: FTESTEXC   r30, r29, INVALID\n"
    "   32: GOTO_IF_Z  r30, L1\n"
    "   33: FCLEAREXC  r31, r29\n"
    "   34: FSETSTATE  r31\n"
    "   35: NOT        r32, r28\n"
    "   36: ORI        r33, r28, 16777216\n"
    "   37: ANDI       r34, r32, 16777216\n"
    "   38: SET_ALIAS  a5, r33\n"
    "   39: GOTO_IF_Z  r34, L2\n"
    "   40: ORI        r35, r33, -2147483648\n"
    "   41: SET_ALIAS  a5, r35\n"
    "   42: LABEL      L2\n"
    "   43: LABEL      L1\n"
    "   44: LOAD_IMM   r36, 4\n"
    "   45: SET_ALIAS  a1, r36\n"
    "   46: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,32] --> 1,4\n"
    "Block 1: 0 --> [33,39] --> 2,3\n"
    "Block 2: 1 --> [40,41] --> 3\n"
    "Block 3: 2,1 --> [42,42] --> 4\n"
    "Block 4: 3,0 --> [43,46] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
