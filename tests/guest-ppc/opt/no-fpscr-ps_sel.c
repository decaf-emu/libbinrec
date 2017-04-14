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
    0x10,0x22,0x20,0xEE,  // ps_sel f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: LOAD_IMM   r6, 0.0\n"
    "    6: VEXTRACT   r7, r3, 0\n"
    "    7: VEXTRACT   r8, r4, 0\n"
    "    8: VEXTRACT   r9, r5, 0\n"
    "    9: FCMP       r10, r7, r6, GE\n"
    "   10: SELECT     r11, r8, r9, r10\n"
    "   11: VEXTRACT   r12, r3, 1\n"
    "   12: VEXTRACT   r13, r4, 1\n"
    "   13: VEXTRACT   r14, r5, 1\n"
    "   14: FCMP       r15, r12, r6, GE\n"
    "   15: SELECT     r16, r13, r14, r15\n"
    "   16: VBUILD2    r17, r11, r16\n"
    "   17: SET_ALIAS  a2, r17\n"
    "   18: LOAD_IMM   r18, 4\n"
    "   19: SET_ALIAS  a1, r18\n"
    "   20: RETURN\n"
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
