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
    0xC0,0x20,0x00,0x10,  // lfs f1,16(0)
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_BR    r3, 16(r2)\n"
    "    3: FCAST      r4, r3\n"
    "    4: STORE      408(r1), r4\n"
    "    5: SET_ALIAS  a2, r4\n"
    "    6: LOAD_IMM   r5, 4\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"