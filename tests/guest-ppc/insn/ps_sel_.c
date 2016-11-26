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
    0x10,0x22,0x20,0xEF,  // ps_sel. f1,f2,f3,f4
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
    "    4: GET_ALIAS  r5, a4\n"
    "    5: GET_ALIAS  r6, a5\n"
    "    6: LOAD_IMM   r7, 0.0\n"
    "    7: VEXTRACT   r8, r4, 0\n"
    "    8: VEXTRACT   r9, r5, 0\n"
    "    9: VEXTRACT   r10, r6, 0\n"
    "   10: FCMP       r11, r8, r7, GE\n"
    "   11: SELECT     r12, r9, r10, r11\n"
    "   12: VEXTRACT   r13, r4, 1\n"
    "   13: VEXTRACT   r14, r5, 1\n"
    "   14: VEXTRACT   r15, r6, 1\n"
    "   15: FCMP       r16, r13, r7, GE\n"
    "   16: SELECT     r17, r14, r15, r16\n"
    "   17: VBUILD2    r18, r12, r17\n"
    "   18: FSETSTATE  r3\n"
    "   19: GET_ALIAS  r19, a7\n"
    "   20: ANDI       r20, r19, 33031936\n"
    "   21: SGTUI      r21, r20, 0\n"
    "   22: BFEXT      r22, r19, 25, 4\n"
    "   23: SLLI       r23, r21, 4\n"
    "   24: SRLI       r24, r19, 3\n"
    "   25: OR         r25, r22, r23\n"
    "   26: AND        r26, r25, r24\n"
    "   27: ANDI       r27, r26, 31\n"
    "   28: SGTUI      r28, r27, 0\n"
    "   29: BFEXT      r29, r19, 31, 1\n"
    "   30: BFEXT      r30, r19, 28, 1\n"
    "   31: GET_ALIAS  r31, a6\n"
    "   32: SLLI       r32, r29, 3\n"
    "   33: SLLI       r33, r28, 2\n"
    "   34: SLLI       r34, r21, 1\n"
    "   35: OR         r35, r32, r33\n"
    "   36: OR         r36, r34, r30\n"
    "   37: OR         r37, r35, r36\n"
    "   38: BFINS      r38, r31, r37, 24, 4\n"
    "   39: SET_ALIAS  a6, r38\n"
    "   40: SET_ALIAS  a2, r18\n"
    "   41: LOAD_IMM   r39, 4\n"
    "   42: SET_ALIAS  a1, r39\n"
    "   43: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,43] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
