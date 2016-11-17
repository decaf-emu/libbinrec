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
    0x10,0x20,0x10,0x31,  // ps_res. f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_NATIVE_RECIPROCAL;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (10201031) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
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
    "   12: VFCAST     r9, r8\n"
    "   13: LOAD_IMM   r10, 1.0f\n"
    "   14: VBROADCAST r11, r10\n"
    "   15: FDIV       r12, r11, r9\n"
    "   16: GET_ALIAS  r13, a9\n"
    "   17: ANDI       r14, r13, 33031936\n"
    "   18: SGTUI      r15, r14, 0\n"
    "   19: BFEXT      r16, r13, 25, 4\n"
    "   20: SLLI       r17, r15, 4\n"
    "   21: SRLI       r18, r13, 3\n"
    "   22: OR         r19, r16, r17\n"
    "   23: AND        r20, r19, r18\n"
    "   24: ANDI       r21, r20, 31\n"
    "   25: SGTUI      r22, r21, 0\n"
    "   26: BFEXT      r23, r13, 31, 1\n"
    "   27: BFEXT      r24, r13, 28, 1\n"
    "   28: SET_ALIAS  a5, r23\n"
    "   29: SET_ALIAS  a6, r22\n"
    "   30: SET_ALIAS  a7, r15\n"
    "   31: SET_ALIAS  a8, r24\n"
    "   32: VFCVT      r25, r12\n"
    "   33: SET_ALIAS  a2, r25\n"
    "   34: LOAD_IMM   r26, 4\n"
    "   35: SET_ALIAS  a1, r26\n"
    "   36: GET_ALIAS  r27, a4\n"
    "   37: ANDI       r28, r27, -251658241\n"
    "   38: GET_ALIAS  r29, a5\n"
    "   39: SLLI       r30, r29, 27\n"
    "   40: OR         r31, r28, r30\n"
    "   41: GET_ALIAS  r32, a6\n"
    "   42: SLLI       r33, r32, 26\n"
    "   43: OR         r34, r31, r33\n"
    "   44: GET_ALIAS  r35, a7\n"
    "   45: SLLI       r36, r35, 25\n"
    "   46: OR         r37, r34, r36\n"
    "   47: GET_ALIAS  r38, a8\n"
    "   48: SLLI       r39, r38, 24\n"
    "   49: OR         r40, r37, r39\n"
    "   50: SET_ALIAS  a4, r40\n"
    "   51: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
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
