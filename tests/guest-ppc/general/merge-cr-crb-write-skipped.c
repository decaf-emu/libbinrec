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
    0x41,0x82,0x00,0x08,  // beq 0x8
    0x4F,0xFF,0xFA,0x42,  // crset 31
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 0, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: ANDI       r6, r5, 536870912\n"
    "    7: GOTO_IF_NZ r6, L1\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: LOAD_IMM   r8, 1\n"
    "   11: SET_ALIAS  a3, r8\n"
    "   12: LOAD_IMM   r9, 8\n"
    "   13: SET_ALIAS  a1, r9\n"
    "   14: LABEL      L1\n"
    "   15: LOAD_IMM   r10, 12\n"
    "   16: SET_ALIAS  a1, r10\n"
    "   17: GET_ALIAS  r11, a2\n"
    "   18: ANDI       r12, r11, -2\n"
    "   19: GET_ALIAS  r13, a3\n"
    "   20: OR         r14, r12, r13\n"
    "   21: SET_ALIAS  a2, r14\n"
    "   22: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,13] --> 2\n"
    "Block 2: 1,0 --> [14,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
