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
    0x10,0x20,0x11,0x10,  // ps_nabs f1,f2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FGETSTATE  r4\n"
    "    4: VFCAST     r5, r3\n"
    "    5: FSETSTATE  r4\n"
    "    6: FNABS      r6, r5\n"
    "    7: VFCAST     r7, r6\n"
    "    8: SET_ALIAS  a2, r7\n"
    "    9: LOAD_IMM   r8, 4\n"
    "   10: SET_ALIAS  a1, r8\n"
    "   11: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
