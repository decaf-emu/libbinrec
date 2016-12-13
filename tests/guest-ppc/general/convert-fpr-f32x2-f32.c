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
    0x10,0x40,0x18,0x90,  // ps_mr f2,f3
    0xEC,0x22,0x10,0x2A,  // fadds f1,f2,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: VFCMP      r4, r3, r3, UN\n"
    "    4: VFCVT      r5, r3\n"
    "    5: SET_ALIAS  a5, r5\n"
    "    6: GOTO_IF_Z  r4, L1\n"
    "    7: VEXTRACT   r6, r3, 0\n"
    "    8: VEXTRACT   r7, r3, 1\n"
    "    9: BFEXT      r8, r4, 0, 32\n"
    "   10: BFEXT      r9, r4, 32, 32\n"
    "   11: ZCAST      r10, r8\n"
    "   12: ZCAST      r11, r9\n"
    "   13: BITCAST    r12, r6\n"
    "   14: BITCAST    r13, r7\n"
    "   15: NOT        r14, r12\n"
    "   16: NOT        r15, r13\n"
    "   17: LOAD_IMM   r16, 0x8000000000000\n"
    "   18: AND        r17, r14, r16\n"
    "   19: AND        r18, r15, r16\n"
    "   20: VEXTRACT   r19, r5, 0\n"
    "   21: VEXTRACT   r20, r5, 1\n"
    "   22: SRLI       r21, r17, 29\n"
    "   23: SRLI       r22, r18, 29\n"
    "   24: ZCAST      r23, r21\n"
    "   25: ZCAST      r24, r22\n"
    "   26: BITCAST    r25, r19\n"
    "   27: BITCAST    r26, r20\n"
    "   28: AND        r27, r23, r10\n"
    "   29: AND        r28, r24, r11\n"
    "   30: XOR        r29, r25, r27\n"
    "   31: XOR        r30, r26, r28\n"
    "   32: BITCAST    r31, r29\n"
    "   33: BITCAST    r32, r30\n"
    "   34: VBUILD2    r33, r31, r32\n"
    "   35: SET_ALIAS  a5, r33\n"
    "   36: LABEL      L1\n"
    "   37: GET_ALIAS  r34, a5\n"
    "   38: VEXTRACT   r35, r34, 0\n"
    "   39: VEXTRACT   r36, r34, 0\n"
    "   40: FADD       r37, r35, r36\n"
    "   41: FCVT       r38, r37\n"
    "   42: STORE      408(r1), r38\n"
    "   43: SET_ALIAS  a2, r38\n"
    "   44: VFCMP      r39, r34, r34, UN\n"
    "   45: VFCVT      r40, r34\n"
    "   46: SET_ALIAS  a6, r40\n"
    "   47: GOTO_IF_Z  r39, L2\n"
    "   48: VEXTRACT   r41, r34, 0\n"
    "   49: VEXTRACT   r42, r34, 1\n"
    "   50: SLLI       r43, r39, 32\n"
    "   51: BITCAST    r44, r41\n"
    "   52: BITCAST    r45, r42\n"
    "   53: NOT        r46, r44\n"
    "   54: NOT        r47, r45\n"
    "   55: ANDI       r48, r46, 4194304\n"
    "   56: ANDI       r49, r47, 4194304\n"
    "   57: VEXTRACT   r50, r40, 0\n"
    "   58: VEXTRACT   r51, r40, 1\n"
    "   59: ZCAST      r52, r48\n"
    "   60: ZCAST      r53, r49\n"
    "   61: SLLI       r54, r52, 29\n"
    "   62: SLLI       r55, r53, 29\n"
    "   63: BITCAST    r56, r50\n"
    "   64: BITCAST    r57, r51\n"
    "   65: AND        r58, r54, r43\n"
    "   66: AND        r59, r55, r39\n"
    "   67: XOR        r60, r56, r58\n"
    "   68: XOR        r61, r57, r59\n"
    "   69: BITCAST    r62, r60\n"
    "   70: BITCAST    r63, r61\n"
    "   71: VBUILD2    r64, r62, r63\n"
    "   72: SET_ALIAS  a6, r64\n"
    "   73: LABEL      L2\n"
    "   74: GET_ALIAS  r65, a6\n"
    "   75: SET_ALIAS  a3, r65\n"
    "   76: LOAD_IMM   r66, 8\n"
    "   77: SET_ALIAS  a1, r66\n"
    "   78: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float32[2], no bound storage\n"
    "Alias 6: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,2\n"
    "Block 1: 0 --> [7,35] --> 2\n"
    "Block 2: 1,0 --> [36,47] --> 3,4\n"
    "Block 3: 2 --> [48,72] --> 4\n"
    "Block 4: 3,2 --> [73,78] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
