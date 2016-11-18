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
    0xFC,0x22,0x20,0xFA,  // fmadd f1,f2,f3,f4
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
    "    5: FMADD      r6, r3, r4, r5\n"
    "    6: LOAD_IMM   r7, 0.0\n"
    "    7: FCMP       r8, r3, r7, NUN\n"
    "    8: FCMP       r9, r4, r7, UN\n"
    "    9: FCMP       r10, r5, r7, UN\n"
    "   10: AND        r11, r8, r9\n"
    "   11: AND        r12, r11, r10\n"
    "   12: BITCAST    r13, r5\n"
    "   13: LOAD_IMM   r14, 0x8000000000000\n"
    "   14: OR         r15, r13, r14\n"
    "   15: BITCAST    r16, r15\n"
    "   16: SELECT     r17, r16, r6, r12\n"
    "   17: SET_ALIAS  a2, r17\n"
    "   18: LOAD_IMM   r18, 4\n"
    "   19: SET_ALIAS  a1, r18\n"
    "   20: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: float64 @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
