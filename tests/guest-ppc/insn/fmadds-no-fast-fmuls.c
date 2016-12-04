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
    0xEC,0x22,0x20,0xFA,  // fmadds f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FMADDS;
static const unsigned int common_opt = BINREC_OPT_NATIVE_IEEE_NAN;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: SET_ALIAS  a6, r3\n"
    "    6: SET_ALIAS  a7, r4\n"
    "    7: BITCAST    r6, r4\n"
    "    8: BITCAST    r7, r3\n"
    "    9: BFEXT      r8, r6, 0, 52\n"
    "   10: GOTO_IF_Z  r8, L1\n"
    "   11: BFEXT      r9, r7, 52, 11\n"
    "   12: BFEXT      r10, r6, 52, 11\n"
    "   13: SEQI       r11, r9, 2047\n"
    "   14: GOTO_IF_NZ r11, L1\n"
    "   15: SEQI       r12, r10, 2047\n"
    "   16: GOTO_IF_NZ r12, L1\n"
    "   17: GOTO_IF_NZ r10, L2\n"
    "   18: CLZ        r13, r8\n"
    "   19: ADDI       r14, r13, -11\n"
    "   20: SUB        r15, r9, r14\n"
    "   21: SGTSI      r16, r15, 0\n"
    "   22: GOTO_IF_Z  r16, L1\n"
    "   23: LOAD_IMM   r17, 0x8000000000000000\n"
    "   24: AND        r18, r6, r17\n"
    "   25: SLL        r19, r8, r14\n"
    "   26: BFINS      r20, r7, r15, 52, 11\n"
    "   27: OR         r21, r18, r19\n"
    "   28: BITCAST    r22, r20\n"
    "   29: BITCAST    r23, r21\n"
    "   30: SET_ALIAS  a6, r22\n"
    "   31: SET_ALIAS  a7, r23\n"
    "   32: LABEL      L2\n"
    "   33: GET_ALIAS  r24, a7\n"
    "   34: BITCAST    r25, r24\n"
    "   35: ANDI       r26, r25, -134217728\n"
    "   36: ANDI       r27, r25, 134217728\n"
    "   37: ADD        r28, r26, r27\n"
    "   38: BITCAST    r29, r28\n"
    "   39: SET_ALIAS  a7, r29\n"
    "   40: BFEXT      r30, r28, 52, 11\n"
    "   41: SEQI       r31, r30, 2047\n"
    "   42: GOTO_IF_Z  r31, L1\n"
    "   43: LOAD_IMM   r32, 0x10000000000000\n"
    "   44: SUB        r33, r28, r32\n"
    "   45: BITCAST    r34, r33\n"
    "   46: SET_ALIAS  a7, r34\n"
    "   47: BFEXT      r35, r7, 52, 11\n"
    "   48: GOTO_IF_Z  r35, L3\n"
    "   49: SGTUI      r36, r35, 2045\n"
    "   50: GOTO_IF_NZ r36, L1\n"
    "   51: ADD        r37, r7, r32\n"
    "   52: BITCAST    r38, r37\n"
    "   53: SET_ALIAS  a6, r38\n"
    "   54: GOTO       L1\n"
    "   55: LABEL      L3\n"
    "   56: SLLI       r39, r7, 1\n"
    "   57: BFINS      r40, r7, r39, 0, 63\n"
    "   58: BITCAST    r41, r40\n"
    "   59: SET_ALIAS  a6, r41\n"
    "   60: LABEL      L1\n"
    "   61: GET_ALIAS  r42, a6\n"
    "   62: GET_ALIAS  r43, a7\n"
    "   63: FMADD      r44, r42, r43, r5\n"
    "   64: FCVT       r45, r44\n"
    "   65: FCVT       r46, r45\n"
    "   66: STORE      408(r1), r46\n"
    "   67: SET_ALIAS  a2, r46\n"
    "   68: LOAD_IMM   r47, 4\n"
    "   69: SET_ALIAS  a1, r47\n"
    "   70: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "Alias 6: float64, no bound storage\n"
    "Alias 7: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,11\n"
    "Block 1: 0 --> [11,14] --> 2,11\n"
    "Block 2: 1 --> [15,16] --> 3,11\n"
    "Block 3: 2 --> [17,17] --> 4,6\n"
    "Block 4: 3 --> [18,22] --> 5,11\n"
    "Block 5: 4 --> [23,31] --> 6\n"
    "Block 6: 5,3 --> [32,42] --> 7,11\n"
    "Block 7: 6 --> [43,48] --> 8,10\n"
    "Block 8: 7 --> [49,50] --> 9,11\n"
    "Block 9: 8 --> [51,54] --> 11\n"
    "Block 10: 7 --> [55,59] --> 11\n"
    "Block 11: 10,9,0,1,2,4,6,8 --> [60,70] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
