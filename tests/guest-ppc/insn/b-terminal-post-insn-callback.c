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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 8\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: LOAD_IMM   r4, 0x2\n"
    "    5: LOAD_IMM   r5, 0\n"
    "    6: CALL_TRANSPARENT @r4, r1, r5\n"
    "    7: GOTO       L1\n"
    "    8: LOAD_IMM   r6, 4\n"
    "    9: SET_ALIAS  a1, r6\n"
    "   10: LOAD_IMM   r7, 0x2\n"
    "   11: LOAD_IMM   r8, 0\n"
    "   12: CALL_TRANSPARENT @r7, r1, r8\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LABEL      L1\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 2\n"
    "Block 1: <none> --> [8,14] --> 2\n"
    "Block 2: 1,0 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
