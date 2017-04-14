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
    0x10,0x84,0x20,0x2A,  // ps_add f4,f4,f4
    0x10,0x22,0x19,0x16,  // ps_sum1 f1,f2,f4,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a5\n"
    "    3: FADD       r4, r3, r3\n"
    "    4: VFCVT      r5, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: VEXTRACT   r7, r6, 0\n"
    "    7: GET_ALIAS  r8, a4\n"
    "    8: VEXTRACT   r9, r8, 1\n"
    "    9: VEXTRACT   r10, r5, 0\n"
    "   10: FADD       r11, r7, r9\n"
    "   11: FCVT       r12, r11\n"
    "   12: VBUILD2    r13, r10, r12\n"
    "   13: VFCVT      r14, r13\n"
    "   14: SET_ALIAS  a2, r14\n"
    "   15: VFCVT      r15, r5\n"
    "   16: SET_ALIAS  a5, r15\n"
    "   17: LOAD_IMM   r16, 8\n"
    "   18: SET_ALIAS  a1, r16\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
