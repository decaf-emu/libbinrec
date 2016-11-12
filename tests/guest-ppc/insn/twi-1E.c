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
    0x0F,0xC3,0x12,0x34,  // twi 30,r3,4660
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 4660\n"
    "    3: GET_ALIAS  r4, a2\n"
    "    4: LOAD_IMM   r5, 0\n"
    "    5: SET_ALIAS  a1, r5\n"
    "    6: LOAD       r6, 984(r1)\n"
    "    7: CALL       @r6, r1\n"
    "    8: RETURN\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> <none>\n"
    "Block 1: <none> --> [9,11] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
