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
    0xFC,0x22,0x20,0xEF,  // fsel. f1,f2,f3,f4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: FGETSTATE  r3\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: LOAD_IMM   r5, 0.0\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: FCMP       r8, r4, r5, GE\n"
    "    8: SELECT     r9, r6, r7, r8\n"
    "    9: FSETSTATE  r3\n"
    "   10: GET_ALIAS  r10, a7\n"
    "   11: ANDI       r11, r10, 33031936\n"
    "   12: SGTUI      r12, r11, 0\n"
    "   13: BFEXT      r13, r10, 25, 4\n"
    "   14: SLLI       r14, r12, 4\n"
    "   15: SRLI       r15, r10, 3\n"
    "   16: OR         r16, r13, r14\n"
    "   17: AND        r17, r16, r15\n"
    "   18: ANDI       r18, r17, 31\n"
    "   19: SGTUI      r19, r18, 0\n"
    "   20: BFEXT      r20, r10, 31, 1\n"
    "   21: BFEXT      r21, r10, 28, 1\n"
    "   22: GET_ALIAS  r22, a6\n"
    "   23: SLLI       r23, r20, 3\n"
    "   24: SLLI       r24, r19, 2\n"
    "   25: SLLI       r25, r12, 1\n"
    "   26: OR         r26, r23, r24\n"
    "   27: OR         r27, r25, r21\n"
    "   28: OR         r28, r26, r27\n"
    "   29: BFINS      r29, r22, r28, 24, 4\n"
    "   30: SET_ALIAS  a6, r29\n"
    "   31: SET_ALIAS  a2, r9\n"
    "   32: LOAD_IMM   r30, 4\n"
    "   33: SET_ALIAS  a1, r30\n"
    "   34: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
