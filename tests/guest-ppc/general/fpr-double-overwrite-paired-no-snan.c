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
                                    | BINREC_OPT_G_PPC_ASSUME_NO_SNAN
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
    "   12: FCVT       r13, r12\n"
    "   13: VBUILD2    r14, r11, r13\n"
    "   14: SET_ALIAS  a3, r14\n"
    "   15: LOAD_IMM   r15, 8\n"
    "   16: SET_ALIAS  a1, r15\n"
    "   17: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
