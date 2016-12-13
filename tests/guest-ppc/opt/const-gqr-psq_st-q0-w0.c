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
    0xF0,0x23,0x2F,0xF0,  // psq_st f1,-16(r3),0,2
};

#define INITIAL_STATE \
    &(PPCInsnTestState){.gqr = {-1,-1, 0x00000000, -1,-1,-1,-1,-1}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_CONSTANT_GQRS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: VEXTRACT   r7, r6, 0\n"
    "    7: BITCAST    r8, r7\n"
    "    8: SRLI       r9, r8, 32\n"
    "    9: ZCAST      r10, r9\n"
    "   10: SLLI       r11, r8, 1\n"
    "   11: GOTO_IF_Z  r11, L1\n"
    "   12: LOAD_IMM   r12, 0x701FFFFFFFFFFFFF\n"
    "   13: SGTU       r13, r11, r12\n"
    "   14: GOTO_IF_Z  r13, L2\n"
    "   15: LABEL      L1\n"
    "   16: ANDI       r14, r10, -1073741824\n"
    "   17: BFEXT      r15, r8, 29, 30\n"
    "   18: ZCAST      r16, r15\n"
    "   19: OR         r17, r14, r16\n"
    "   20: STORE_BR   -16(r5), r17\n"
    "   21: GOTO       L3\n"
    "   22: LABEL      L2\n"
    "   23: LOAD_IMM   r18, 0\n"
    "   24: STORE      -16(r5), r18\n"
    "   25: LABEL      L3\n"
    "   26: VEXTRACT   r19, r6, 1\n"
    "   27: BITCAST    r20, r19\n"
    "   28: SRLI       r21, r20, 32\n"
    "   29: ZCAST      r22, r21\n"
    "   30: SLLI       r23, r20, 1\n"
    "   31: GOTO_IF_Z  r23, L4\n"
    "   32: LOAD_IMM   r24, 0x701FFFFFFFFFFFFF\n"
    "   33: SGTU       r25, r23, r24\n"
    "   34: GOTO_IF_Z  r25, L5\n"
    "   35: LABEL      L4\n"
    "   36: ANDI       r26, r22, -1073741824\n"
    "   37: BFEXT      r27, r20, 29, 30\n"
    "   38: ZCAST      r28, r27\n"
    "   39: OR         r29, r26, r28\n"
    "   40: STORE_BR   -12(r5), r29\n"
    "   41: GOTO       L6\n"
    "   42: LABEL      L5\n"
    "   43: LOAD_IMM   r30, 0\n"
    "   44: STORE      -12(r5), r30\n"
    "   45: LABEL      L6\n"
    "   46: LOAD_IMM   r31, 4\n"
    "   47: SET_ALIAS  a1, r31\n"
    "   48: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,14] --> 2,3\n"
    "Block 2: 1,0 --> [15,21] --> 4\n"
    "Block 3: 1 --> [22,24] --> 4\n"
    "Block 4: 3,2 --> [25,31] --> 5,6\n"
    "Block 5: 4 --> [32,34] --> 6,7\n"
    "Block 6: 5,4 --> [35,41] --> 8\n"
    "Block 7: 5 --> [42,44] --> 8\n"
    "Block 8: 7,6 --> [45,48] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
