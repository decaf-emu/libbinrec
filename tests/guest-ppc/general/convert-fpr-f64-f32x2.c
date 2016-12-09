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
    0xFC,0x40,0x18,0x90,  // fmr f2,f3
    0x10,0x20,0x10,0x90,  // ps_mr f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: FGETSTATE  r4\n"
    "    4: GET_ALIAS  r5, a3\n"
    "    5: VINSERT    r6, r5, r3, 0\n"
    "    6: VFCAST     r7, r6\n"
    "    7: FSETSTATE  r4\n"
    "    8: VFCAST     r8, r7\n"
    "    9: SET_ALIAS  a2, r8\n"
    "   10: GET_ALIAS  r9, a3\n"
    "   11: VINSERT    r10, r9, r3, 0\n"
    "   12: SET_ALIAS  a3, r10\n"
    "   13: LOAD_IMM   r11, 8\n"
    "   14: SET_ALIAS  a1, r11\n"
    "   15: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,15] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
