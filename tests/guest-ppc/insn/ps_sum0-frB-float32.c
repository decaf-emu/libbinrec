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
    0x10,0x63,0x18,0x2A,  // ps_add f3,f3,f3
    0x10,0x22,0x19,0x14,  // ps_sum0 f1,f2,f4,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: FADD       r4, r3, r3\n"
    "    4: VFCVT      r5, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: VEXTRACT   r7, r6, 0\n"
    "    7: VEXTRACT   r8, r5, 1\n"
    "    8: FCVT       r9, r8\n"
    "    9: GET_ALIAS  r10, a5\n"
    "   10: VEXTRACT   r11, r10, 1\n"
    "   11: FCAST      r12, r11\n"
    "   12: FADD       r13, r7, r9\n"
    "   13: FCVT       r14, r13\n"
    "   14: VBUILD2    r15, r14, r12\n"
    "   15: VFCAST     r16, r15\n"
    "   16: SET_ALIAS  a2, r16\n"
    "   17: VFCVT      r17, r5\n"
    "   18: SET_ALIAS  a4, r17\n"
    "   19: LOAD_IMM   r18, 8\n"
    "   20: SET_ALIAS  a1, r18\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
