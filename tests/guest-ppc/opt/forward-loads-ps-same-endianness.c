/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#define TEST_PPC_HOST_BIG_ENDIAN
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
    "    6: SRLI       r7, r6, 32\n"
    "    7: ZCAST      r8, r7\n"
    "    8: ZCAST      r9, r6\n"
    "    9: BITCAST    r10, r8\n"
    "   10: BITCAST    r11, r9\n"
    "   11: VBUILD2    r12, r10, r11\n"
    "   12: ZCAST      r13, r3\n"
    "   13: ADD        r14, r2, r13\n"
    "   14: STORE      -8(r14), r6\n"
    "   15: VFCVT      r15, r12\n"
    "   16: SET_ALIAS  a3, r15\n"
    "   17: LOAD_IMM   r16, 8\n"
    "   18: SET_ALIAS  a1, r16\n"
    "   19: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
