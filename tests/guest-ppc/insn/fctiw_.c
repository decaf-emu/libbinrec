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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FCTIW;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (FC20101D) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FROUNDI    r4, r3\n"
    "    4: ZCAST      r5, r4\n"
    "    5: BITCAST    r6, r5\n"
    "    6: FGETSTATE  r7\n"
    "    7: FTESTEXC   r8, r7, INVALID\n"
    "    8: GOTO_IF_Z  r8, L1\n"
    "    9: LOAD_IMM   r9, 0.0\n"
    "   10: FCMP       r10, r3, r9, GT\n"
    "   11: GOTO_IF_Z  r10, L1\n"
    "   12: LOAD_IMM   r11, 1.0609978949885705e-314\n"
    "   13: SET_ALIAS  a2, r11\n"
    "   14: GOTO       L2\n"
    "   15: LABEL      L1\n"
    "   16: SET_ALIAS  a2, r6\n"
    "   17: LABEL      L2\n"
    "   18: FCLEAREXC  r12, r7\n"
    "   19: FSETSTATE  r12\n"
    "   20: GET_ALIAS  r13, a5\n"
    "   21: ANDI       r14, r13, 33031936\n"
    "   22: SGTUI      r15, r14, 0\n"
    "   23: BFEXT      r16, r13, 25, 4\n"
    "   24: SLLI       r17, r15, 4\n"
    "   25: SRLI       r18, r13, 3\n"
    "   26: OR         r19, r16, r17\n"
    "   27: AND        r20, r19, r18\n"
    "   28: ANDI       r21, r20, 31\n"
    "   29: SGTUI      r22, r21, 0\n"
    "   30: BFEXT      r23, r13, 31, 1\n"
    "   31: BFEXT      r24, r13, 28, 1\n"
    "   32: GET_ALIAS  r25, a4\n"
    "   33: SLLI       r26, r23, 3\n"
    "   34: SLLI       r27, r22, 2\n"
    "   35: SLLI       r28, r15, 1\n"
    "   36: OR         r29, r26, r27\n"
    "   37: OR         r30, r28, r24\n"
    "   38: OR         r31, r29, r30\n"
    "   39: BFINS      r32, r25, r31, 24, 4\n"
    "   40: SET_ALIAS  a4, r32\n"
    "   41: LOAD_IMM   r33, 4\n"
    "   42: SET_ALIAS  a1, r33\n"
    "   43: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,3\n"
    "Block 1: 0 --> [9,11] --> 2,3\n"
    "Block 2: 1 --> [12,14] --> 4\n"
    "Block 3: 0,1 --> [15,16] --> 4\n"
    "Block 4: 3,2 --> [17,43] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
