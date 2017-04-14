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
    0x10,0x20,0x10,0x90,  // ps_mr f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VFCMP      r4, r3, r3, UN\n"
    "    4: VFCVT      r5, r3\n"
    "    5: SET_ALIAS  a4, r5\n"
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
    "   35: SET_ALIAS  a4, r33\n"
    "   36: LABEL      L1\n"
    "   37: GET_ALIAS  r34, a4\n"
    "   38: VFCMP      r35, r34, r34, UN\n"
    "   39: VFCVT      r36, r34\n"
    "   40: SET_ALIAS  a5, r36\n"
    "   41: GOTO_IF_Z  r35, L2\n"
    "   42: VEXTRACT   r37, r34, 0\n"
    "   43: VEXTRACT   r38, r34, 1\n"
    "   44: SLLI       r39, r35, 32\n"
    "   45: BITCAST    r40, r37\n"
    "   46: BITCAST    r41, r38\n"
    "   47: NOT        r42, r40\n"
    "   48: NOT        r43, r41\n"
    "   49: ANDI       r44, r42, 4194304\n"
    "   50: ANDI       r45, r43, 4194304\n"
    "   51: VEXTRACT   r46, r36, 0\n"
    "   52: VEXTRACT   r47, r36, 1\n"
    "   53: ZCAST      r48, r44\n"
    "   54: ZCAST      r49, r45\n"
    "   55: SLLI       r50, r48, 29\n"
    "   56: SLLI       r51, r49, 29\n"
    "   57: BITCAST    r52, r46\n"
    "   58: BITCAST    r53, r47\n"
    "   59: AND        r54, r50, r39\n"
    "   60: AND        r55, r51, r35\n"
    "   61: XOR        r56, r52, r54\n"
    "   62: XOR        r57, r53, r55\n"
    "   63: BITCAST    r58, r56\n"
    "   64: BITCAST    r59, r57\n"
    "   65: VBUILD2    r60, r58, r59\n"
    "   66: SET_ALIAS  a5, r60\n"
    "   67: LABEL      L2\n"
    "   68: GET_ALIAS  r61, a5\n"
    "   69: SET_ALIAS  a2, r61\n"
    "   70: LOAD_IMM   r62, 4\n"
    "   71: SET_ALIAS  a1, r62\n"
    "   72: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float32[2], no bound storage\n"
    "Alias 5: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,2\n"
    "Block 1: 0 --> [7,35] --> 2\n"
    "Block 2: 1,0 --> [36,41] --> 3,4\n"
    "Block 3: 2 --> [42,66] --> 4\n"
    "Block 4: 3,2 --> [67,72] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
