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
    0xFC,0x20,0x10,0x1C,  // fctiw f1,f2
    0x10,0x20,0x08,0x50,  // ps_neg f1,f1
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
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
    "   13: GET_ALIAS  r12, a2\n"
    "   14: VINSERT    r13, r12, r11, 0\n"
    "   15: SET_ALIAS  a2, r13\n"
    "   16: GOTO       L2\n"
    "   17: LABEL      L1\n"
    "   18: GET_ALIAS  r14, a2\n"
    "   19: VINSERT    r15, r14, r6, 0\n"
    "   20: SET_ALIAS  a2, r15\n"
    "   21: LABEL      L2\n"
    "   22: GET_ALIAS  r16, a2\n"
    "   23: VEXTRACT   r17, r16, 0\n"
    "   24: BITCAST    r18, r3\n"
    "   25: BITCAST    r19, r17\n"
    "   26: LOAD_IMM   r20, 0x8000000000000000\n"
    "   27: SEQ        r21, r18, r20\n"
    "   28: SLLI       r22, r21, 32\n"
    "   29: LOAD_IMM   r23, 0xFFF8000000000000\n"
    "   30: OR         r24, r23, r22\n"
    "   31: OR         r25, r19, r24\n"
    "   32: BITCAST    r26, r25\n"
    "   33: VINSERT    r27, r16, r26, 0\n"
    "   34: SET_ALIAS  a2, r27\n"
    "   35: FCLEAREXC  r28, r7\n"
    "   36: FSETSTATE  r28\n"
    "   37: GET_ALIAS  r29, a2\n"
    "   38: VFCAST     r30, r29\n"
    "   39: FNEG       r31, r30\n"
    "   40: VFCAST     r32, r31\n"
    "   41: SET_ALIAS  a2, r32\n"
    "   42: LOAD_IMM   r33, 8\n"
    "   43: SET_ALIAS  a1, r33\n"
    "   44: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,3\n"
    "Block 1: 0 --> [9,11] --> 2,3\n"
    "Block 2: 1 --> [12,16] --> 4\n"
    "Block 3: 0,1 --> [17,20] --> 4\n"
    "Block 4: 3,2 --> [21,44] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
