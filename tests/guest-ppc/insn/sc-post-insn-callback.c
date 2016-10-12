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

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LOAD       r4, 976(r1)\n"
    "    6: CALL       @r4, r1\n"
    "    7: LOAD_IMM   r5, 0x2\n"
    "    8: LOAD_IMM   r6, 0\n"
    "    9: CALL_TRANSPARENT @r5, r1, r6\n"
    "   10: RETURN\n"
    "   11: LOAD_IMM   r7, 4\n"
    "   12: SET_ALIAS  a1, r7\n"
    "   13: LOAD_IMM   r8, 0x2\n"
    "   14: LOAD_IMM   r9, 0\n"
    "   15: CALL_TRANSPARENT @r8, r1, r9\n"
    "   16: LOAD_IMM   r10, 4\n"
    "   17: SET_ALIAS  a1, r10\n"
    "   18: LABEL      L2\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,10] --> <none>\n"
    "Block 2: <none> --> [11,17] --> 3\n"
    "Block 3: 2 --> [18,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
