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
    "   26: LOAD_IMM   r19, 4\n"
    "   27: SET_ALIAS  a1, r19\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,14] --> 2,3\n"
    "Block 2: 1,0 --> [15,21] --> 4\n"
    "Block 3: 1 --> [22,24] --> 4\n"
    "Block 4: 3,2 --> [25,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
