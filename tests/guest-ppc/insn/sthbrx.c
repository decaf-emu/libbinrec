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
    0x7C,0x64,0x2F,0x2C,  // sthbrx r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: ADD        r5, r3, r4\n"
    "    6: ZCAST      r6, r5\n"
    "    7: ADD        r7, r2, r6\n"
    "    8: GET_ALIAS  r8, a2\n"
    "    9: STORE_I16  0(r7), r8\n"
    "   10: LOAD_IMM   r9, 4\n"
    "   11: SET_ALIAS  a1, r9\n"
    "   12: LABEL      L2\n"
    "   13: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,11] --> 2\n"
    "Block 2: 1 --> [12,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"