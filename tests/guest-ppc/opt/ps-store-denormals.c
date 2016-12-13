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
    0x10,0x21,0x0C,0x60,  // ps_merge01 f1,f1,f1
    0xF0,0x23,0x2F,0xF0,  // psq_st f1,-16(r3),0,2
};

#define INITIAL_STATE \
    &(PPCInsnTestState){.gqr = {-1,-1, 0x00000000, -1,-1,-1,-1,-1}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_CONSTANT_GQRS
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_PS_STORE_DENORMALS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VEXTRACT   r4, r3, 0\n"
    "    4: FCVT       r5, r4\n"
    "    5: VEXTRACT   r6, r3, 1\n"
    "    6: FCVT       r7, r6\n"
    "    7: VBUILD2    r8, r5, r7\n"
    "    8: GET_ALIAS  r9, a2\n"
    "    9: ZCAST      r10, r9\n"
    "   10: ADD        r11, r2, r10\n"
    "   11: VEXTRACT   r12, r8, 0\n"
    "   12: STORE_BR   -16(r11), r12\n"
    "   13: VEXTRACT   r13, r8, 1\n"
    "   14: STORE_BR   -12(r11), r13\n"
    "   15: VFCVT      r14, r8\n"
    "   16: SET_ALIAS  a3, r14\n"
    "   17: LOAD_IMM   r15, 8\n"
    "   18: SET_ALIAS  a1, r15\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
