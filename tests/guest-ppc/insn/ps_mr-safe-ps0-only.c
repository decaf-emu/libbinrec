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
    0xFC,0x43,0x20,0x2A,  // fadd f2,f3,f4
    0x10,0x20,0x10,0x90,  // ps_mr f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: GET_ALIAS  r4, a5\n"
    "    4: FADD       r5, r3, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: VINSERT    r7, r6, r5, 0\n"
    "    7: VFCAST     r8, r7\n"
    "    8: VFCAST     r9, r8\n"
    "    9: SET_ALIAS  a2, r9\n"
    "   10: GET_ALIAS  r10, a3\n"
    "   11: VINSERT    r11, r10, r5, 0\n"
    "   12: SET_ALIAS  a3, r11\n"
    "   13: LOAD_IMM   r12, 8\n"
    "   14: SET_ALIAS  a1, r12\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
