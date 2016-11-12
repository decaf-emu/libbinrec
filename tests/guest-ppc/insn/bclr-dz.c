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
    0x4E,0x40,0x00,0x20,  // bdzlr
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, -4\n"
    "    4: GET_ALIAS  r5, a3\n"
    "    5: ADDI       r6, r5, -1\n"
    "    6: SET_ALIAS  a3, r6\n"
    "    7: GOTO_IF_NZ r6, L1\n"
    "    8: SET_ALIAS  a1, r4\n"
    "    9: GOTO       L2\n"
    "   10: LABEL      L1\n"
    "   11: LOAD_IMM   r7, 4\n"
    "   12: SET_ALIAS  a1, r7\n"
    "   13: LABEL      L2\n"
    "   14: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 932(r1)\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,9] --> 3\n"
    "Block 2: 0 --> [10,12] --> 3\n"
    "Block 3: 2,1 --> [13,14] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
