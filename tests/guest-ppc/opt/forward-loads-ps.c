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
    0xE0,0x23,0x0F,0xF0,  // psq_l f1,-16(r3),0,0
    0xF0,0x23,0x0F,0xF8,  // psq_st f1,-8(r3),0,0
};

#define INITIAL_STATE \
    &(PPCInsnTestState){.gqr = {0x00000000, -1,-1,-1,-1,-1,-1,-1}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_FORWARD_LOADS
                                    | BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_CONSTANT_GQRS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, -16(r5)\n"
    "    6: BSWAP      r7, r6\n"
    "    7: SRLI       r8, r7, 32\n"
    "    8: ZCAST      r9, r8\n"
    "    9: ZCAST      r10, r7\n"
    "   10: BITCAST    r11, r9\n"
    "   11: BITCAST    r12, r10\n"
    "   12: VBUILD2    r13, r11, r12\n"
    "   13: ZCAST      r14, r3\n"
    "   14: ADD        r15, r2, r14\n"
    "   15: STORE      -8(r15), r6\n"
    "   16: VFCVT      r16, r13\n"
    "   17: SET_ALIAS  a3, r16\n"
    "   18: LOAD_IMM   r17, 8\n"
    "   19: SET_ALIAS  a1, r17\n"
    "   20: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
