/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define POST_INSN_CALLBACK  ((void (*)(void *, uint32_t))2)

static const uint8_t input[] = {
    0x44,0x00,0x00,0x02,  // sc
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 4\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: LOAD       r4, 976(r1)\n"
    "    5: CALL       @r4, r1\n"
    "    6: LOAD_IMM   r5, 0x2\n"
    "    7: LOAD_IMM   r6, 0\n"
    "    8: CALL_TRANSPARENT @r5, r1, r6\n"
    "    9: RETURN\n"
    "   10: LOAD_IMM   r7, 4\n"
    "   11: SET_ALIAS  a1, r7\n"
    "   12: LOAD_IMM   r8, 0x2\n"
    "   13: LOAD_IMM   r9, 0\n"
    "   14: CALL_TRANSPARENT @r8, r1, r9\n"
    "   15: LOAD_IMM   r10, 4\n"
    "   16: SET_ALIAS  a1, r10\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> <none>\n"
    "Block 1: <none> --> [10,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
