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
    "    6: FGETSTATE  r7\n"
    "    7: VEXTRACT   r8, r6, 0\n"
    "    8: FCAST      r9, r8\n"
    "    9: FSETSTATE  r7\n"
    "   10: BITCAST    r10, r9\n"
    "   11: BFEXT      r11, r10, 23, 8\n"
    "   12: ANDI       r12, r10, -2147483648\n"
    "   13: BITCAST    r13, r12\n"
    "   14: SELECT     r14, r9, r13, r11\n"
    "   15: STORE_BR   -16(r5), r14\n"
    "   16: LOAD_IMM   r15, 4\n"
    "   17: SET_ALIAS  a1, r15\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
