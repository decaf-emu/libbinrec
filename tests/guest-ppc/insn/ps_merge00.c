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
    0x10,0x22,0x1C,0x20,  // ps_merge00 f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FGETSTATE  r4\n"
    "    4: FCVT       r5, r3\n"
    "    5: FSETSTATE  r4\n"
    "    6: GET_ALIAS  r6, a4\n"
    "    7: FGETSTATE  r7\n"
    "    8: FSETROUND  r8, r7, TRUNC\n"
    "    9: FSETSTATE  r8\n"
    "   10: FCVT       r9, r6\n"
    "   11: FSETSTATE  r7\n"
    "   12: VBUILD2    r10, r5, r9\n"
    "   13: VFCVT      r11, r10\n"
    "   14: SET_ALIAS  a2, r11\n"
    "   15: LOAD_IMM   r12, 4\n"
    "   16: SET_ALIAS  a1, r12\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
