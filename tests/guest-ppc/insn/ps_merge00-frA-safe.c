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
    0xFC,0x42,0x10,0x2A,  // fadd f2,f2,f2
    0x10,0x22,0x1C,0x20,  // ps_merge00 f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FADD       r4, r3, r3\n"
    "    4: FCVT       r5, r4\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: FGETSTATE  r7\n"
    "    7: FSETROUND  r8, r7, TRUNC\n"
    "    8: FSETSTATE  r8\n"
    "    9: FCMP       r9, r6, r6, UN\n"
    "   10: FCVT       r10, r6\n"
    "   11: SET_ALIAS  a5, r10\n"
    "   12: GOTO_IF_Z  r9, L1\n"
    "   13: BITCAST    r11, r6\n"
    "   14: BFEXT      r12, r11, 51, 1\n"
    "   15: GOTO_IF_NZ r12, L1\n"
    "   16: BITCAST    r13, r10\n"
    "   17: XORI       r14, r13, 4194304\n"
    "   18: BITCAST    r15, r14\n"
    "   19: SET_ALIAS  a5, r15\n"
    "   20: LABEL      L1\n"
    "   21: GET_ALIAS  r16, a5\n"
    "   22: FSETSTATE  r7\n"
    "   23: VBUILD2    r17, r5, r16\n"
    "   24: VFCMP      r18, r17, r17, UN\n"
    "   25: VFCVT      r19, r17\n"
    "   26: SET_ALIAS  a6, r19\n"
    "   27: GOTO_IF_Z  r18, L2\n"
    "   28: VEXTRACT   r20, r17, 0\n"
    "   29: VEXTRACT   r21, r17, 1\n"
    "   30: SLLI       r22, r18, 32\n"
    "   31: BITCAST    r23, r20\n"
    "   32: BITCAST    r24, r21\n"
    "   33: NOT        r25, r23\n"
    "   34: NOT        r26, r24\n"
    "   35: ANDI       r27, r25, 4194304\n"
    "   36: ANDI       r28, r26, 4194304\n"
    "   37: VEXTRACT   r29, r19, 0\n"
    "   38: VEXTRACT   r30, r19, 1\n"
    "   39: ZCAST      r31, r27\n"
    "   40: ZCAST      r32, r28\n"
    "   41: SLLI       r33, r31, 29\n"
    "   42: SLLI       r34, r32, 29\n"
    "   43: BITCAST    r35, r29\n"
    "   44: BITCAST    r36, r30\n"
    "   45: AND        r37, r33, r22\n"
    "   46: AND        r38, r34, r18\n"
    "   47: XOR        r39, r35, r37\n"
    "   48: XOR        r40, r36, r38\n"
    "   49: BITCAST    r41, r39\n"
    "   50: BITCAST    r42, r40\n"
    "   51: VBUILD2    r43, r41, r42\n"
    "   52: SET_ALIAS  a6, r43\n"
    "   53: LABEL      L2\n"
    "   54: GET_ALIAS  r44, a6\n"
    "   55: SET_ALIAS  a2, r44\n"
    "   56: SET_ALIAS  a3, r4\n"
    "   57: LOAD_IMM   r45, 8\n"
    "   58: SET_ALIAS  a1, r45\n"
    "   59: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float32, no bound storage\n"
    "Alias 6: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,3\n"
    "Block 1: 0 --> [13,15] --> 2,3\n"
    "Block 2: 1 --> [16,19] --> 3\n"
    "Block 3: 2,0,1 --> [20,27] --> 4,5\n"
    "Block 4: 3 --> [28,52] --> 5\n"
    "Block 5: 4,3 --> [53,59] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
