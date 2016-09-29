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
    0x44,0x00,0x00,0x02,  // sc
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LOAD       r4, 976(r1)\n"
    "    6: CALL_ADDR  @r4, r1\n"
    "    7: RETURN\n"
    "    8: LOAD_IMM   r5, 8\n"
    "    9: SET_ALIAS  a1, r5\n"
    "   10: LABEL      L2\n"
    "   11: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,7] --> <none>\n"
    "Block 2: <none> --> [8,9] --> 3\n"
    "Block 3: 2 --> [10,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
