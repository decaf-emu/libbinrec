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
    0xEC,0x20,0x10,0x31,  // fres. f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_NATIVE_RECIPROCAL;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (EC201031) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
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
    "   12: FCAST      r9, r8\n"
    "   13: LOAD_IMM   r10, 1.0f\n"
    "   14: FDIV       r11, r10, r9\n"
    "   15: GET_ALIAS  r12, a9\n"
    "   16: ANDI       r13, r12, 33031936\n"
    "   17: SGTUI      r14, r13, 0\n"
    "   18: BFEXT      r15, r12, 25, 4\n"
    "   19: SLLI       r16, r14, 4\n"
    "   20: SRLI       r17, r12, 3\n"
    "   21: OR         r18, r15, r16\n"
    "   22: AND        r19, r18, r17\n"
    "   23: ANDI       r20, r19, 31\n"
    "   24: SGTUI      r21, r20, 0\n"
    "   25: BFEXT      r22, r12, 31, 1\n"
    "   26: BFEXT      r23, r12, 28, 1\n"
    "   27: SET_ALIAS  a5, r22\n"
    "   28: SET_ALIAS  a6, r21\n"
    "   29: SET_ALIAS  a7, r14\n"
    "   30: SET_ALIAS  a8, r23\n"
    "   31: FCVT       r24, r11\n"
    "   32: STORE      408(r1), r24\n"
    "   33: SET_ALIAS  a2, r24\n"
    "   34: LOAD_IMM   r25, 4\n"
    "   35: SET_ALIAS  a1, r25\n"
    "   36: GET_ALIAS  r26, a4\n"
    "   37: ANDI       r27, r26, -251658241\n"
    "   38: GET_ALIAS  r28, a5\n"
    "   39: SLLI       r29, r28, 27\n"
    "   40: OR         r30, r27, r29\n"
    "   41: GET_ALIAS  r31, a6\n"
    "   42: SLLI       r32, r31, 26\n"
    "   43: OR         r33, r30, r32\n"
    "   44: GET_ALIAS  r34, a7\n"
    "   45: SLLI       r35, r34, 25\n"
    "   46: OR         r36, r33, r35\n"
    "   47: GET_ALIAS  r37, a8\n"
    "   48: SLLI       r38, r37, 24\n"
    "   49: OR         r39, r36, r38\n"
    "   50: SET_ALIAS  a4, r39\n"
    "   51: RETURN\n"
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
    "Block 0: <none> --> [0,51] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
