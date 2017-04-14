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
    0xF0,0x23,0xAF,0xF0,  // psq_st f1,-16(r3),1,2
};

#define INITIAL_STATE  &(PPCInsnTestState){.gqr = {0,0,0x00001104}}

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
    "    5: LOAD_IMM   r6, 131072.0f\n"
    "    6: LOAD_IMM   r7, 0\n"
    "    7: LOAD_IMM   r8, 255\n"
    "    8: FGETSTATE  r9\n"
    "    9: GET_ALIAS  r10, a3\n"
    "   10: VEXTRACT   r11, r10, 0\n"
    "   11: FCVT       r12, r11\n"
    "   12: FMUL       r13, r12, r6\n"
    "   13: BITCAST    r14, r13\n"
    "   14: FTRUNCI    r15, r13\n"
    "   15: SLLI       r16, r14, 1\n"
    "   16: SRLI       r17, r14, 31\n"
    "   17: SELECT     r18, r7, r8, r17\n"
    "   18: SGTUI      r19, r16, -1895825409\n"
    "   19: SELECT     r20, r18, r15, r19\n"
    "   20: SGTS       r21, r20, r8\n"
    "   21: SELECT     r22, r8, r20, r21\n"
    "   22: SLTS       r23, r20, r7\n"
    "   23: SELECT     r24, r7, r22, r23\n"
    "   24: FSETSTATE  r9\n"
    "   25: STORE_I8   -16(r5), r24\n"
    "   26: LOAD_IMM   r25, 4\n"
    "   27: SET_ALIAS  a1, r25\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
