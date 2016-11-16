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
    0x10,0x22,0x19,0x15,  // ps_sum0. f1,f2,f4,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (10221915) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a6\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a7, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a8, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a9, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a10, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: VEXTRACT   r9, r8, 0\n"
    "   13: GET_ALIAS  r10, a4\n"
    "   14: VEXTRACT   r11, r10, 1\n"
    "   15: GET_ALIAS  r12, a5\n"
    "   16: VEXTRACT   r13, r12, 1\n"
    "   17: FCAST      r14, r13\n"
    "   18: FADD       r15, r9, r11\n"
    "   19: FCVT       r16, r15\n"
    "   20: VBUILD2    r17, r16, r14\n"
    "   21: GET_ALIAS  r18, a11\n"
    "   22: ANDI       r19, r18, 33031936\n"
    "   23: SGTUI      r20, r19, 0\n"
    "   24: BFEXT      r21, r18, 25, 4\n"
    "   25: SLLI       r22, r20, 4\n"
    "   26: SRLI       r23, r18, 3\n"
    "   27: OR         r24, r21, r22\n"
    "   28: AND        r25, r24, r23\n"
    "   29: ANDI       r26, r25, 31\n"
    "   30: SGTUI      r27, r26, 0\n"
    "   31: BFEXT      r28, r18, 31, 1\n"
    "   32: BFEXT      r29, r18, 28, 1\n"
    "   33: SET_ALIAS  a7, r28\n"
    "   34: SET_ALIAS  a8, r27\n"
    "   35: SET_ALIAS  a9, r20\n"
    "   36: SET_ALIAS  a10, r29\n"
    "   37: VFCVT      r30, r17\n"
    "   38: SET_ALIAS  a2, r30\n"
    "   39: LOAD_IMM   r31, 4\n"
    "   40: SET_ALIAS  a1, r31\n"
    "   41: GET_ALIAS  r32, a6\n"
    "   42: ANDI       r33, r32, -251658241\n"
    "   43: GET_ALIAS  r34, a7\n"
    "   44: SLLI       r35, r34, 27\n"
    "   45: OR         r36, r33, r35\n"
    "   46: GET_ALIAS  r37, a8\n"
    "   47: SLLI       r38, r37, 26\n"
    "   48: OR         r39, r36, r38\n"
    "   49: GET_ALIAS  r40, a9\n"
    "   50: SLLI       r41, r40, 25\n"
    "   51: OR         r42, r39, r41\n"
    "   52: GET_ALIAS  r43, a10\n"
    "   53: SLLI       r44, r43, 24\n"
    "   54: OR         r45, r42, r44\n"
    "   55: SET_ALIAS  a6, r45\n"
    "   56: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,56] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
