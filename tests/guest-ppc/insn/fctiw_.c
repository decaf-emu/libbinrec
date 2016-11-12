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
    0xFC,0x20,0x10,0x1D,  // fctiw. f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (FC20101D) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
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
    "   12: FROUNDI    r9, r8\n"
    "   13: ZCAST      r10, r9\n"
    "   14: BITCAST    r11, r10\n"
    "   15: FGETSTATE  r12\n"
    "   16: FTESTEXC   r13, r12, INVALID\n"
    "   17: GOTO_IF_Z  r13, L1\n"
    "   18: LOAD_IMM   r14, 0.0\n"
    "   19: FCMP       r15, r8, r14, GT\n"
    "   20: GOTO_IF_Z  r15, L1\n"
    "   21: LOAD_IMM   r16, 1.0609978949885705e-314\n"
    "   22: SET_ALIAS  a2, r16\n"
    "   23: GOTO       L2\n"
    "   24: LABEL      L1\n"
    "   25: SET_ALIAS  a2, r11\n"
    "   26: LABEL      L2\n"
    "   27: FCLEAREXC  r17, r12\n"
    "   28: FSETSTATE  r17\n"
    "   29: GET_ALIAS  r18, a9\n"
    "   30: ANDI       r19, r18, 33031936\n"
    "   31: SGTUI      r20, r19, 0\n"
    "   32: BFEXT      r21, r18, 25, 4\n"
    "   33: SLLI       r22, r20, 4\n"
    "   34: SRLI       r23, r18, 3\n"
    "   35: OR         r24, r21, r22\n"
    "   36: AND        r25, r24, r23\n"
    "   37: ANDI       r26, r25, 31\n"
    "   38: SGTUI      r27, r26, 0\n"
    "   39: BFEXT      r28, r18, 31, 1\n"
    "   40: BFEXT      r29, r18, 28, 1\n"
    "   41: SET_ALIAS  a5, r28\n"
    "   42: SET_ALIAS  a6, r27\n"
    "   43: SET_ALIAS  a7, r20\n"
    "   44: SET_ALIAS  a8, r29\n"
    "   45: LOAD_IMM   r30, 4\n"
    "   46: SET_ALIAS  a1, r30\n"
    "   47: GET_ALIAS  r31, a4\n"
    "   48: ANDI       r32, r31, -251658241\n"
    "   49: GET_ALIAS  r33, a5\n"
    "   50: SLLI       r34, r33, 27\n"
    "   51: OR         r35, r32, r34\n"
    "   52: GET_ALIAS  r36, a6\n"
    "   53: SLLI       r37, r36, 26\n"
    "   54: OR         r38, r35, r37\n"
    "   55: GET_ALIAS  r39, a7\n"
    "   56: SLLI       r40, r39, 25\n"
    "   57: OR         r41, r38, r40\n"
    "   58: GET_ALIAS  r42, a8\n"
    "   59: SLLI       r43, r42, 24\n"
    "   60: OR         r44, r41, r43\n"
    "   61: SET_ALIAS  a4, r44\n"
    "   62: RETURN\n"
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
    "Block 0: <none> --> [0,17] --> 1,3\n"
    "Block 1: 0 --> [18,20] --> 2,3\n"
    "Block 2: 1 --> [21,23] --> 4\n"
    "Block 3: 0,1 --> [24,25] --> 4\n"
    "Block 4: 3,2 --> [26,62] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
