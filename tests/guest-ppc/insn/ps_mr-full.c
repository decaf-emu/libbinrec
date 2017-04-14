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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FGETSTATE  r4\n"
    "    4: VFCMP      r5, r3, r3, UN\n"
    "    5: VFCVT      r6, r3\n"
    "    6: SET_ALIAS  a4, r6\n"
    "    7: GOTO_IF_Z  r5, L1\n"
    "    8: VEXTRACT   r7, r3, 0\n"
    "    9: VEXTRACT   r8, r3, 1\n"
    "   10: BFEXT      r9, r5, 0, 32\n"
    "   11: BFEXT      r10, r5, 32, 32\n"
    "   12: ZCAST      r11, r9\n"
    "   13: ZCAST      r12, r10\n"
    "   14: BITCAST    r13, r7\n"
    "   15: BITCAST    r14, r8\n"
    "   16: NOT        r15, r13\n"
    "   17: NOT        r16, r14\n"
    "   18: LOAD_IMM   r17, 0x8000000000000\n"
    "   19: AND        r18, r15, r17\n"
    "   20: AND        r19, r16, r17\n"
    "   21: VEXTRACT   r20, r6, 0\n"
    "   22: VEXTRACT   r21, r6, 1\n"
    "   23: SRLI       r22, r18, 29\n"
    "   24: SRLI       r23, r19, 29\n"
    "   25: ZCAST      r24, r22\n"
    "   26: ZCAST      r25, r23\n"
    "   27: BITCAST    r26, r20\n"
    "   28: BITCAST    r27, r21\n"
    "   29: AND        r28, r24, r11\n"
    "   30: AND        r29, r25, r12\n"
    "   31: XOR        r30, r26, r28\n"
    "   32: XOR        r31, r27, r29\n"
    "   33: BITCAST    r32, r30\n"
    "   34: BITCAST    r33, r31\n"
    "   35: VBUILD2    r34, r32, r33\n"
    "   36: SET_ALIAS  a4, r34\n"
    "   37: LABEL      L1\n"
    "   38: GET_ALIAS  r35, a4\n"
    "   39: FSETSTATE  r4\n"
    "   40: FGETSTATE  r36\n"
    "   41: VFCMP      r37, r35, r35, UN\n"
    "   42: VFCVT      r38, r35\n"
    "   43: SET_ALIAS  a5, r38\n"
    "   44: GOTO_IF_Z  r37, L2\n"
    "   45: VEXTRACT   r39, r35, 0\n"
    "   46: VEXTRACT   r40, r35, 1\n"
    "   47: SLLI       r41, r37, 32\n"
    "   48: BITCAST    r42, r39\n"
    "   49: BITCAST    r43, r40\n"
    "   50: NOT        r44, r42\n"
    "   51: NOT        r45, r43\n"
    "   52: ANDI       r46, r44, 4194304\n"
    "   53: ANDI       r47, r45, 4194304\n"
    "   54: VEXTRACT   r48, r38, 0\n"
    "   55: VEXTRACT   r49, r38, 1\n"
    "   56: ZCAST      r50, r46\n"
    "   57: ZCAST      r51, r47\n"
    "   58: SLLI       r52, r50, 29\n"
    "   59: SLLI       r53, r51, 29\n"
    "   60: BITCAST    r54, r48\n"
    "   61: BITCAST    r55, r49\n"
    "   62: AND        r56, r52, r41\n"
    "   63: AND        r57, r53, r37\n"
    "   64: XOR        r58, r54, r56\n"
    "   65: XOR        r59, r55, r57\n"
    "   66: BITCAST    r60, r58\n"
    "   67: BITCAST    r61, r59\n"
    "   68: VBUILD2    r62, r60, r61\n"
    "   69: SET_ALIAS  a5, r62\n"
    "   70: LABEL      L2\n"
    "   71: GET_ALIAS  r63, a5\n"
    "   72: FSETSTATE  r36\n"
    "   73: SET_ALIAS  a2, r63\n"
    "   74: LOAD_IMM   r64, 4\n"
    "   75: SET_ALIAS  a1, r64\n"
    "   76: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float32[2], no bound storage\n"
    "Alias 5: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,36] --> 2\n"
    "Block 2: 1,0 --> [37,44] --> 3,4\n"
    "Block 3: 2 --> [45,69] --> 4\n"
    "Block 4: 3,2 --> [70,76] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
