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
    0xEC,0x40,0x10,0x30,  // fres f2,f2
    0x10,0x22,0x18,0x2A,  // ps_add f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_NATIVE_RECIPROCAL
                                    | BINREC_OPT_G_PPC_FAST_NANS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VEXTRACT   r4, r3, 0\n"
    "    4: FCVT       r5, r4\n"
    "    5: LOAD_IMM   r6, 1.0f\n"
    "    6: FDIV       r7, r6, r5\n"
    "    7: FCVT       r8, r7\n"
    "    8: VBROADCAST r9, r8\n"
    "    9: GET_ALIAS  r10, a4\n"
    "   10: FADD       r11, r9, r10\n"
    "   11: VFCVT      r12, r11\n"
    "   12: VFCVT      r13, r12\n"
    "   13: SET_ALIAS  a2, r13\n"
    "   14: FCVT       r14, r7\n"
    "   15: VBROADCAST r15, r14\n"
    "   16: SET_ALIAS  a3, r15\n"
    "   17: LOAD_IMM   r16, 8\n"
    "   18: SET_ALIAS  a1, r16\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
