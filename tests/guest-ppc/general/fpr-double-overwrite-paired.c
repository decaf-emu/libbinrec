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
    0xE0,0x23,0x00,0x00,  // psq_l f1,0(r3),0,0
    0xC8,0x23,0x00,0x08,  // lfd f1,8(r3)
};

#define INITIAL_STATE  &(PPCInsnTestState){.gqr = {0}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_CONSTANT_GQRS
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, 0(r5)\n"
    "    6: LOAD_BR    r7, 4(r5)\n"
    "    7: VBUILD2    r8, r6, r7\n"
    "    8: ZCAST      r9, r3\n"
    "    9: ADD        r10, r2, r9\n"
    "   10: LOAD_BR    r11, 8(r10)\n"
    "   11: VEXTRACT   r12, r8, 1\n"
    "   12: FCMP       r13, r12, r12, UN\n"
    "   13: FCVT       r14, r12\n"
    "   14: SET_ALIAS  a4, r14\n"
    "   15: GOTO_IF_Z  r13, L1\n"
    "   16: BITCAST    r15, r12\n"
    "   17: ANDI       r16, r15, 4194304\n"
    "   18: GOTO_IF_NZ r16, L1\n"
    "   19: BITCAST    r17, r14\n"
    "   20: LOAD_IMM   r18, 0x8000000000000\n"
    "   21: XOR        r19, r17, r18\n"
    "   22: BITCAST    r20, r19\n"
    "   23: SET_ALIAS  a4, r20\n"
    "   24: LABEL      L1\n"
    "   25: GET_ALIAS  r21, a4\n"
    "   26: VBUILD2    r22, r11, r21\n"
    "   27: SET_ALIAS  a3, r22\n"
    "   28: LOAD_IMM   r23, 8\n"
    "   29: SET_ALIAS  a1, r23\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,15] --> 1,3\n"
    "Block 1: 0 --> [16,18] --> 2,3\n"
    "Block 2: 1 --> [19,23] --> 3\n"
    "Block 3: 2,0,1 --> [24,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
