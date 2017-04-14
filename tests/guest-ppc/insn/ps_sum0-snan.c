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
    0x10,0x22,0x19,0x14,  // ps_sum0 f1,f2,f4,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VEXTRACT   r4, r3, 0\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: VEXTRACT   r6, r5, 1\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: VEXTRACT   r8, r7, 1\n"
    "    8: FCMP       r9, r8, r8, UN\n"
    "    9: FCVT       r10, r8\n"
    "   10: SET_ALIAS  a6, r10\n"
    "   11: GOTO_IF_Z  r9, L1\n"
    "   12: BITCAST    r11, r8\n"
    "   13: BFEXT      r12, r11, 51, 1\n"
    "   14: GOTO_IF_NZ r12, L1\n"
    "   15: BITCAST    r13, r10\n"
    "   16: XORI       r14, r13, 4194304\n"
    "   17: BITCAST    r15, r14\n"
    "   18: SET_ALIAS  a6, r15\n"
    "   19: LABEL      L1\n"
    "   20: GET_ALIAS  r16, a6\n"
    "   21: FADD       r17, r4, r6\n"
    "   22: FCVT       r18, r17\n"
    "   23: VBUILD2    r19, r18, r16\n"
    "   24: VFCMP      r20, r19, r19, UN\n"
    "   25: VFCVT      r21, r19\n"
    "   26: SET_ALIAS  a7, r21\n"
    "   27: GOTO_IF_Z  r20, L2\n"
    "   28: VEXTRACT   r22, r19, 0\n"
    "   29: VEXTRACT   r23, r19, 1\n"
    "   30: SLLI       r24, r20, 32\n"
    "   31: BITCAST    r25, r22\n"
    "   32: BITCAST    r26, r23\n"
    "   33: NOT        r27, r25\n"
    "   34: NOT        r28, r26\n"
    "   35: ANDI       r29, r27, 4194304\n"
    "   36: ANDI       r30, r28, 4194304\n"
    "   37: VEXTRACT   r31, r21, 0\n"
    "   38: VEXTRACT   r32, r21, 1\n"
    "   39: ZCAST      r33, r29\n"
    "   40: ZCAST      r34, r30\n"
    "   41: SLLI       r35, r33, 29\n"
    "   42: SLLI       r36, r34, 29\n"
    "   43: BITCAST    r37, r31\n"
    "   44: BITCAST    r38, r32\n"
    "   45: AND        r39, r35, r24\n"
    "   46: AND        r40, r36, r20\n"
    "   47: XOR        r41, r37, r39\n"
    "   48: XOR        r42, r38, r40\n"
    "   49: BITCAST    r43, r41\n"
    "   50: BITCAST    r44, r42\n"
    "   51: VBUILD2    r45, r43, r44\n"
    "   52: SET_ALIAS  a7, r45\n"
    "   53: LABEL      L2\n"
    "   54: GET_ALIAS  r46, a7\n"
    "   55: SET_ALIAS  a2, r46\n"
    "   56: LOAD_IMM   r47, 4\n"
    "   57: SET_ALIAS  a1, r47\n"
    "   58: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: float32, no bound storage\n"
    "Alias 7: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,3\n"
    "Block 1: 0 --> [12,14] --> 2,3\n"
    "Block 2: 1 --> [15,18] --> 3\n"
    "Block 3: 2,0,1 --> [19,27] --> 4,5\n"
    "Block 4: 3 --> [28,52] --> 5\n"
    "Block 5: 4,3 --> [53,58] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
