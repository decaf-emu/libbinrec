/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define CHAINING

static const uint8_t input[] = {
    0x48,0x00,0x00,0x09,  // bl 0x8
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 8\n"
    "    3: LOAD_IMM   r4, 4\n"
    "    4: SET_ALIAS  a2, r4\n"
    "    5: SET_ALIAS  a1, r3\n"
    "    6: CHAIN      r1, r2\n"
    "    7: LOAD       r5, 1000(r1)\n"
    "    8: CALL       r6, @r5, r1, r3\n"
    "    9: CHAIN_RESOLVE @6, r6\n"
    "   10: RETURN     r1\n"
    "   11: LOAD_IMM   r7, 4\n"
    "   12: SET_ALIAS  a1, r7\n"
    "   13: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> <none>\n"
    "Block 1: <none> --> [11,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
