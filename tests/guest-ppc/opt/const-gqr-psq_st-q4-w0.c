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

#define INITIAL_STATE  &(PPCInsnTestState){.gqr = {0,0,0x00000004}}

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
    "    5: LOAD_IMM   r6, 0\n"
    "    6: LOAD_IMM   r7, 255\n"
    "    7: FGETSTATE  r8\n"
    "    8: GET_ALIAS  r9, a3\n"
    "    9: VFCVT      r10, r9\n"
    "   10: VEXTRACT   r11, r10, 0\n"
    "   11: BITCAST    r12, r11\n"
    "   12: FROUNDI    r13, r11\n"
    "   13: SLLI       r14, r12, 1\n"
    "   14: SRLI       r15, r12, 31\n"
    "   15: SELECT     r16, r6, r7, r15\n"
    "   16: SGTUI      r17, r14, -1895825409\n"
    "   17: SELECT     r18, r16, r13, r17\n"
    "   18: SGTS       r19, r18, r7\n"
    "   19: SELECT     r20, r7, r18, r19\n"
    "   20: SLTS       r21, r18, r6\n"
    "   21: SELECT     r22, r6, r20, r21\n"
    "   22: VEXTRACT   r23, r10, 1\n"
    "   23: BITCAST    r24, r23\n"
    "   24: FROUNDI    r25, r23\n"
    "   25: SLLI       r26, r24, 1\n"
    "   26: SRLI       r27, r24, 31\n"
    "   27: SELECT     r28, r6, r7, r27\n"
    "   28: SGTUI      r29, r26, -1895825409\n"
    "   29: SELECT     r30, r28, r25, r29\n"
    "   30: SGTS       r31, r30, r7\n"
    "   31: SELECT     r32, r7, r30, r31\n"
    "   32: SLTS       r33, r30, r6\n"
    "   33: SELECT     r34, r6, r32, r33\n"
    "   34: FSETSTATE  r8\n"
    "   35: STORE_I8   -16(r5), r22\n"
    "   36: STORE_I8   -15(r5), r34\n"
    "   37: LOAD_IMM   r35, 4\n"
    "   38: SET_ALIAS  a1, r35\n"
    "   39: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,39] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
