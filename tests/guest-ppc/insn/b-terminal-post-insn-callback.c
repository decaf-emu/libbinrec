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
    0x48,0x00,0x00,0x08,  // b 0x8
};

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 8\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LOAD_IMM   r4, 0x2\n"
    "    6: LOAD_IMM   r5, 0\n"
    "    7: CALL_TRANSPARENT @r4, r1, r5\n"
    "    8: GOTO       L2\n"
    "    9: LOAD_IMM   r6, 4\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: LOAD_IMM   r7, 0x2\n"
    "   12: LOAD_IMM   r8, 0\n"
    "   13: CALL_TRANSPARENT @r7, r1, r8\n"
    "   14: LOAD_IMM   r9, 4\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L2\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,8] --> 3\n"
    "Block 2: <none> --> [9,15] --> 3\n"
    "Block 3: 2,1 --> [16,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
