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
    0x41,0x9E,0x00,0x08,  // beq cr7,0x10
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 3\n"
        "[info] r3 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: NOP\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a4, r4\n"
    "    6: GOTO_IF_Z  r3, L1\n"
    "    7: LOAD_IMM   r5, 16\n"
    "    8: SET_ALIAS  a3, r3\n"
    "    9: SET_ALIAS  a1, r5\n"
    "   10: GOTO       L2\n"
    "   11: LABEL      L1\n"
    "   12: LOAD_IMM   r6, 12\n"
    "   13: SET_ALIAS  a1, r6\n"
    "   14: LOAD_IMM   r7, 0\n"
    "   15: SET_ALIAS  a3, r7\n"
    "   16: LOAD_IMM   r8, 16\n"
    "   17: SET_ALIAS  a1, r8\n"
    "   18: LABEL      L2\n"
    "   19: GET_ALIAS  r9, a2\n"
    "   20: ANDI       r10, r9, -4\n"
    "   21: GET_ALIAS  r11, a3\n"
    "   22: SLLI       r12, r11, 1\n"
    "   23: OR         r13, r10, r12\n"
    "   24: GET_ALIAS  r14, a4\n"
    "   25: OR         r15, r13, r14\n"
    "   26: SET_ALIAS  a2, r15\n"
    "   27: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,2\n"
    "Block 1: 0 --> [7,10] --> 3\n"
    "Block 2: 0 --> [11,17] --> 3\n"
    "Block 3: 2,1 --> [18,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
