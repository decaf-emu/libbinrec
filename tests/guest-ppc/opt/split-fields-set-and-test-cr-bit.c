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
    0x4F,0xC0,0x02,0x42,  // crset 30
    0x48,0x00,0x00,0x04,  // b 0x8
    0x41,0x9E,0xFF,0xF8,  // beq cr7,0x0
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: LABEL      L1\n"
    "    6: LOAD_IMM   r5, 1\n"
    "    7: SET_ALIAS  a3, r5\n"
    "    8: GOTO       L2\n"
    "    9: LOAD_IMM   r6, 8\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: LABEL      L2\n"
    "   12: GET_ALIAS  r7, a3\n"
    "   13: GOTO_IF_NZ r7, L1\n"
    "   14: LOAD_IMM   r8, 12\n"
    "   15: SET_ALIAS  a1, r8\n"
    "   16: GET_ALIAS  r9, a2\n"
    "   17: ANDI       r10, r9, -3\n"
    "   18: GET_ALIAS  r11, a3\n"
    "   19: SLLI       r12, r11, 1\n"
    "   20: OR         r13, r10, r12\n"
    "   21: SET_ALIAS  a2, r13\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0,3 --> [5,8] --> 3\n"
    "Block 2: <none> --> [9,10] --> 3\n"
    "Block 3: 2,1 --> [11,13] --> 4,1\n"
    "Block 4: 3 --> [14,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
