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
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
    /* This loop should not cause infinite recursion. */
    0x41,0x9E,0xFF,0xF8,  // beq cr7,0x0
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 4\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Killing instruction 6\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] Killing instruction 5\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 1\n"
    "    4: NOP\n"
    "    5: NOP\n"
    "    6: NOP\n"
    "    7: GOTO_IF_NZ r3, L1\n"
    "    8: LOAD_IMM   r4, 0\n"
    "    9: SET_ALIAS  a4, r4\n"
    "   10: LOAD_IMM   r5, 12\n"
    "   11: SET_ALIAS  a1, r5\n"
    "   12: LOAD_IMM   r6, 0\n"
    "   13: SET_ALIAS  a3, r6\n"
    "   14: LOAD_IMM   r7, 16\n"
    "   15: SET_ALIAS  a1, r7\n"
    "   16: GET_ALIAS  r8, a2\n"
    "   17: ANDI       r9, r8, -4\n"
    "   18: GET_ALIAS  r10, a3\n"
    "   19: SLLI       r11, r10, 1\n"
    "   20: OR         r12, r9, r11\n"
    "   21: GET_ALIAS  r13, a4\n"
    "   22: OR         r14, r12, r13\n"
    "   23: SET_ALIAS  a2, r14\n"
    "   24: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0,1 --> [2,7] --> 2,1\n"
    "Block 2: 1 --> [8,24] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
