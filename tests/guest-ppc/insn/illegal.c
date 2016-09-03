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
    0x00, 0x00, 0x00, 0x00,  // illegal
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: ILLEGAL\n"
    "    3: LOAD_IMM   r2, 4\n"
    "    4: SET_ALIAS  a1, r2\n"
    "    5: LABEL      L2\n"
    "    6: GET_ALIAS  r3, a1\n"
    "    7: STORE_I32  956(r1), r3\n"
    "    8: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,0] --> 1\n"
    "Block    1: 0 --> [1,4] --> 2\n"
    "Block    2: 1 --> [5,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
