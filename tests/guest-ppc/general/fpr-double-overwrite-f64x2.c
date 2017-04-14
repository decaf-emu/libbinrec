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
    0xFC,0x21,0x08,0x2A,  // fadd f1,f1,f1
    0xF0,0x23,0x00,0x00,  // psq_st f1,0(r3),0,0
};

#define INITIAL_STATE  &(PPCInsnTestState){.gqr = {0}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_CONSTANT_GQRS
                                    | BINREC_OPT_G_PPC_FAST_STFS
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
    "    4: VEXTRACT   r5, r3, 0\n"
    "    5: FADD       r6, r4, r5\n"
    "    6: VINSERT    r7, r3, r6, 0\n"
    "    7: GET_ALIAS  r8, a2\n"
    "    8: ZCAST      r9, r8\n"
    "    9: ADD        r10, r2, r9\n"
    "   10: VFCVT      r11, r7\n"
    "   11: VEXTRACT   r12, r11, 0\n"
    "   12: STORE_BR   0(r10), r12\n"
    "   13: VEXTRACT   r13, r11, 1\n"
    "   14: STORE_BR   4(r10), r13\n"
    "   15: SET_ALIAS  a3, r7\n"
    "   16: LOAD_IMM   r14, 8\n"
    "   17: SET_ALIAS  a1, r14\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
