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
    0x10,0x22,0x1C,0x60,  // ps_merge01 f1,f2,f3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FCAST      r4, r3\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: VEXTRACT   r6, r5, 1\n"
    "    6: FCAST      r7, r6\n"
    "    7: VBUILD2    r8, r4, r7\n"
    "    8: VFCAST     r9, r8\n"
    "    9: SET_ALIAS  a2, r9\n"
    "   10: LOAD_IMM   r10, 4\n"
    "   11: SET_ALIAS  a1, r10\n"
    "   12: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
