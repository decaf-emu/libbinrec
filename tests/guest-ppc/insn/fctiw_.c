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
    "   27: FCLEAREXC\n"
    "   28: GET_ALIAS  r17, a9\n"
    "   29: ANDI       r18, r17, 33031936\n"
    "   30: SGTUI      r19, r18, 0\n"
    "   31: BFEXT      r20, r17, 25, 4\n"
    "   32: SLLI       r21, r19, 4\n"
    "   33: SRLI       r22, r17, 3\n"
    "   34: OR         r23, r20, r21\n"
    "   35: AND        r24, r23, r22\n"
    "   36: ANDI       r25, r24, 31\n"
    "   37: SGTUI      r26, r25, 0\n"
    "   38: BFEXT      r27, r17, 31, 1\n"
    "   39: BFEXT      r28, r17, 28, 1\n"
    "   40: SET_ALIAS  a5, r27\n"
    "   41: SET_ALIAS  a6, r26\n"
    "   42: SET_ALIAS  a7, r19\n"
    "   43: SET_ALIAS  a8, r28\n"
    "   44: LOAD_IMM   r29, 4\n"
    "   45: SET_ALIAS  a1, r29\n"
    "   46: GET_ALIAS  r30, a4\n"
    "   47: ANDI       r31, r30, -251658241\n"
    "   48: GET_ALIAS  r32, a5\n"
    "   49: SLLI       r33, r32, 27\n"
    "   50: OR         r34, r31, r33\n"
    "   51: GET_ALIAS  r35, a6\n"
    "   52: SLLI       r36, r35, 26\n"
    "   53: OR         r37, r34, r36\n"
    "   54: GET_ALIAS  r38, a7\n"
    "   55: SLLI       r39, r38, 25\n"
    "   56: OR         r40, r37, r39\n"
    "   57: GET_ALIAS  r41, a8\n"
    "   58: SLLI       r42, r41, 24\n"
    "   59: OR         r43, r40, r42\n"
    "   60: SET_ALIAS  a4, r43\n"
    "   61: RETURN\n"
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
    "Block 4: 3,2 --> [26,61] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
