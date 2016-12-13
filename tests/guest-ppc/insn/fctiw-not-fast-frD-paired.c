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
    "    4: LOAD_IMM   r5, 2147483647.0\n"
    "    5: FCMP       r6, r3, r5, GE\n"
    "    6: LOAD_IMM   r7, 0x7FFFFFFF\n"
    "    7: SELECT     r8, r7, r4, r6\n"
    "    8: ZCAST      r9, r8\n"
    "    9: BITCAST    r10, r3\n"
    "   10: LOAD_IMM   r11, 0x8000000000000000\n"
    "   11: SEQ        r12, r10, r11\n"
    "   12: SLLI       r13, r12, 32\n"
    "   13: LOAD_IMM   r14, 0xFFF8000000000000\n"
    "   14: OR         r15, r14, r13\n"
    "   15: OR         r16, r9, r15\n"
    "   16: BITCAST    r17, r16\n"
    "   17: GET_ALIAS  r18, a2\n"
    "   18: VINSERT    r19, r18, r17, 0\n"
    "   19: VFCMP      r20, r19, r19, UN\n"
    "   20: VFCVT      r21, r19\n"
    "   21: SET_ALIAS  a4, r21\n"
    "   22: GOTO_IF_Z  r20, L1\n"
    "   23: VEXTRACT   r22, r19, 0\n"
    "   24: VEXTRACT   r23, r19, 1\n"
    "   25: BFEXT      r24, r20, 0, 32\n"
    "   26: BFEXT      r25, r20, 32, 32\n"
    "   27: ZCAST      r26, r24\n"
    "   28: ZCAST      r27, r25\n"
    "   29: BITCAST    r28, r22\n"
    "   30: BITCAST    r29, r23\n"
    "   31: NOT        r30, r28\n"
    "   32: NOT        r31, r29\n"
    "   33: LOAD_IMM   r32, 0x8000000000000\n"
    "   34: AND        r33, r30, r32\n"
    "   35: AND        r34, r31, r32\n"
    "   36: VEXTRACT   r35, r21, 0\n"
    "   37: VEXTRACT   r36, r21, 1\n"
    "   38: SRLI       r37, r33, 29\n"
    "   39: SRLI       r38, r34, 29\n"
    "   40: ZCAST      r39, r37\n"
    "   41: ZCAST      r40, r38\n"
    "   42: BITCAST    r41, r35\n"
    "   43: BITCAST    r42, r36\n"
    "   44: AND        r43, r39, r26\n"
    "   45: AND        r44, r40, r27\n"
    "   46: XOR        r45, r41, r43\n"
    "   47: XOR        r46, r42, r44\n"
    "   48: BITCAST    r47, r45\n"
    "   49: BITCAST    r48, r46\n"
    "   50: VBUILD2    r49, r47, r48\n"
    "   51: SET_ALIAS  a4, r49\n"
    "   52: LABEL      L1\n"
    "   53: GET_ALIAS  r50, a4\n"
    "   54: FNEG       r51, r50\n"
    "   55: VFCMP      r52, r51, r51, UN\n"
    "   56: VFCVT      r53, r51\n"
    "   57: SET_ALIAS  a5, r53\n"
    "   58: GOTO_IF_Z  r52, L2\n"
    "   59: VEXTRACT   r54, r51, 0\n"
    "   60: VEXTRACT   r55, r51, 1\n"
    "   61: SLLI       r56, r52, 32\n"
    "   62: BITCAST    r57, r54\n"
    "   63: BITCAST    r58, r55\n"
    "   64: NOT        r59, r57\n"
    "   65: NOT        r60, r58\n"
    "   66: ANDI       r61, r59, 4194304\n"
    "   67: ANDI       r62, r60, 4194304\n"
    "   68: VEXTRACT   r63, r53, 0\n"
    "   69: VEXTRACT   r64, r53, 1\n"
    "   70: ZCAST      r65, r61\n"
    "   71: ZCAST      r66, r62\n"
    "   72: SLLI       r67, r65, 29\n"
    "   73: SLLI       r68, r66, 29\n"
    "   74: BITCAST    r69, r63\n"
    "   75: BITCAST    r70, r64\n"
    "   76: AND        r71, r67, r56\n"
    "   77: AND        r72, r68, r52\n"
    "   78: XOR        r73, r69, r71\n"
    "   79: XOR        r74, r70, r72\n"
    "   80: BITCAST    r75, r73\n"
    "   81: BITCAST    r76, r74\n"
    "   82: VBUILD2    r77, r75, r76\n"
    "   83: SET_ALIAS  a5, r77\n"
    "   84: LABEL      L2\n"
    "   85: GET_ALIAS  r78, a5\n"
    "   86: SET_ALIAS  a2, r78\n"
    "   87: LOAD_IMM   r79, 8\n"
    "   88: SET_ALIAS  a1, r79\n"
    "   89: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float32[2], no bound storage\n"
    "Alias 5: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,22] --> 1,2\n"
    "Block 1: 0 --> [23,51] --> 2\n"
    "Block 2: 1,0 --> [52,58] --> 3,4\n"
    "Block 3: 2 --> [59,83] --> 4\n"
    "Block 4: 3,2 --> [84,89] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
