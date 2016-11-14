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
    0x4F,0xDE,0xF1,0x82,  // crclr 30
    0x40,0x9E,0x00,0x10,  // bne cr7,0x14
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
    /* The fall-through edge should avoid infinite recursion. */
    0x41,0x9E,0xFF,0xF8,  // beq cr7,0x8
    0x4B,0xFF,0xFF,0xF4,  // b 0x8
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 3\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] Killing instruction 9\n"
        "[info] r5 no longer used, setting death = birth\n"
        "[info] Killing instruction 11\n"
        "[info] r6 no longer used, setting death = birth\n"
        "[info] Killing instruction 10\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 0\n"
    "    3: NOP\n"
    "    4: GOTO_IF_Z  r3, L2\n"
    "    5: LOAD_IMM   r4, 8\n"
    "    6: SET_ALIAS  a1, r4\n"
    "    7: LABEL      L1\n"
    "    8: LOAD_IMM   r5, 1\n"
    "    9: NOP\n"
    "   10: NOP\n"
    "   11: NOP\n"
    "   12: GOTO_IF_NZ r5, L1\n"
    "   13: LOAD_IMM   r7, 20\n"
    "   14: SET_ALIAS  a1, r7\n"
    "   15: LABEL      L2\n"
    "   16: GOTO       L1\n"
    "   17: LOAD_IMM   r8, 24\n"
    "   18: SET_ALIAS  a1, r8\n"
    "   19: GET_ALIAS  r9, a2\n"
    "   20: ANDI       r10, r9, -4\n"
    "   21: GET_ALIAS  r11, a3\n"
    "   22: SLLI       r12, r11, 1\n"
    "   23: OR         r13, r10, r12\n"
    "   24: GET_ALIAS  r14, a4\n"
    "   25: OR         r15, r13, r14\n"
    "   26: SET_ALIAS  a2, r15\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1,4\n"
    "Block 1: 0 --> [5,6] --> 2\n"
    "Block 2: 1,2,4 --> [7,12] --> 3,2\n"
    "Block 3: 2 --> [13,14] --> 4\n"
    "Block 4: 3,0 --> [15,16] --> 2\n"
    "Block 5: <none> --> [17,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
