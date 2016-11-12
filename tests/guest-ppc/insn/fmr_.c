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
    0xFC,0x20,0x10,0x91,  // fmr. f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: GET_ALIAS  r9, a9\n"
    "   13: ANDI       r10, r9, 33031936\n"
    "   14: SGTUI      r11, r10, 0\n"
    "   15: BFEXT      r12, r9, 25, 4\n"
    "   16: SLLI       r13, r11, 4\n"
    "   17: SRLI       r14, r9, 3\n"
    "   18: OR         r15, r12, r13\n"
    "   19: AND        r16, r15, r14\n"
    "   20: ANDI       r17, r16, 31\n"
    "   21: SGTUI      r18, r17, 0\n"
    "   22: BFEXT      r19, r9, 31, 1\n"
    "   23: BFEXT      r20, r9, 28, 1\n"
    "   24: SET_ALIAS  a5, r19\n"
    "   25: SET_ALIAS  a6, r18\n"
    "   26: SET_ALIAS  a7, r11\n"
    "   27: SET_ALIAS  a8, r20\n"
    "   28: SET_ALIAS  a2, r8\n"
    "   29: LOAD_IMM   r21, 4\n"
    "   30: SET_ALIAS  a1, r21\n"
    "   31: GET_ALIAS  r22, a4\n"
    "   32: ANDI       r23, r22, -251658241\n"
    "   33: GET_ALIAS  r24, a5\n"
    "   34: SLLI       r25, r24, 27\n"
    "   35: OR         r26, r23, r25\n"
    "   36: GET_ALIAS  r27, a6\n"
    "   37: SLLI       r28, r27, 26\n"
    "   38: OR         r29, r26, r28\n"
    "   39: GET_ALIAS  r30, a7\n"
    "   40: SLLI       r31, r30, 25\n"
    "   41: OR         r32, r29, r31\n"
    "   42: GET_ALIAS  r33, a8\n"
    "   43: SLLI       r34, r33, 24\n"
    "   44: OR         r35, r32, r34\n"
    "   45: SET_ALIAS  a4, r35\n"
    "   46: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,46] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
