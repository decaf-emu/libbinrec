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
    0xE0,0x23,0x2F,0xF0,  // psq_l f1,-16(r3),0,2
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
    "    5: LOAD_BR    r6, -16(r5)\n"
    "    6: LOAD_BR    r7, -12(r5)\n"
    "    7: VBUILD2    r8, r6, r7\n"
    "    8: VFCAST     r9, r8\n"
    "    9: SET_ALIAS  a3, r9\n"
    "   10: LOAD_IMM   r10, 4\n"
    "   11: SET_ALIAS  a1, r10\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
