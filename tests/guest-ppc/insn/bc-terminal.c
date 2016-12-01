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
    0x60,0x00,0x00,0x00,  // nop
    0x41,0x42,0x81,0x00,  // bc 10,2,0xFFFF8104
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ADDI       r4, r3, -1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GOTO_IF_NZ r4, L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ANDI       r6, r5, 536870912\n"
    "    8: GOTO_IF_Z  r6, L1\n"
    "    9: LOAD_IMM   r7, -32508\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: GOTO       L2\n"
    "   12: LABEL      L1\n"
    "   13: LOAD_IMM   r8, 8\n"
    "   14: SET_ALIAS  a1, r8\n"
    "   15: LABEL      L2\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> 1,3\n"
    "Block 1: 0 --> [6,8] --> 2,3\n"
    "Block 2: 1 --> [9,11] --> 4\n"
    "Block 3: 0,1 --> [12,14] --> 4\n"
    "Block 4: 3,2 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
