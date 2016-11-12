/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define BRANCH_CALLBACK

static const uint8_t input[] = {
    0x48,0x00,0x00,0x08,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_IMM   r3, 8\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: LOAD       r4, 992(r1)\n"
    "    5: LOAD_IMM   r5, 0\n"
    "    6: CALL       r6, @r4, r1, r5\n"
    "    7: GOTO_IF_Z  r6, L2\n"
    "    8: GOTO       L1\n"
    "    9: LOAD_IMM   r7, 4\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: LOAD_IMM   r8, 8\n"
    "   12: SET_ALIAS  a1, r8\n"
    "   13: LABEL      L1\n"
    "   14: LOAD_IMM   r9, 12\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L2\n"
    "   17: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,4\n"
    "Block 1: 0 --> [8,8] --> 3\n"
    "Block 2: <none> --> [9,12] --> 3\n"
    "Block 3: 2,1 --> [13,15] --> 4\n"
    "Block 4: 3,0 --> [16,17] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
