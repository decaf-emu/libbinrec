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
    0x10,0x40,0x18,0x90,  // ps_mr f2,f3
    0xEC,0x22,0x10,0x2A,  // fadds f1,f2,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: VFCAST     r4, r3\n"
    "    4: VEXTRACT   r5, r4, 0\n"
    "    5: VEXTRACT   r6, r4, 0\n"
    "    6: FADD       r7, r5, r6\n"
    "    7: FCVT       r8, r7\n"
    "    8: STORE      408(r1), r8\n"
    "    9: SET_ALIAS  a2, r8\n"
    "   10: VFCAST     r9, r4\n"
    "   11: SET_ALIAS  a3, r9\n"
    "   12: LOAD_IMM   r10, 8\n"
    "   13: SET_ALIAS  a1, r10\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
