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
    0x10,0x42,0x10,0x2A,  // ps_add f2,f2,f2
    0x10,0x22,0x19,0x14,  // ps_sum0 f1,f2,f4,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FADD       r4, r3, r3\n"
    "    4: VFCVT      r5, r4\n"
    "    5: VEXTRACT   r6, r5, 0\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: VEXTRACT   r8, r7, 1\n"
    "    8: FCVT       r9, r8\n"
    "    9: GET_ALIAS  r10, a5\n"
    "   10: VEXTRACT   r11, r10, 1\n"
    "   11: FCVT       r12, r11\n"
    "   12: FADD       r13, r6, r9\n"
    "   13: VBUILD2    r14, r13, r12\n"
    "   14: VFCVT      r15, r14\n"
    "   15: SET_ALIAS  a2, r15\n"
    "   16: VFCVT      r16, r5\n"
    "   17: SET_ALIAS  a3, r16\n"
    "   18: LOAD_IMM   r17, 8\n"
    "   19: SET_ALIAS  a1, r17\n"
    "   20: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
