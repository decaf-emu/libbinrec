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
    0x4D,0x9E,0x00,0x20,  // beqlr cr7
    0x4F,0xFF,0xF9,0x82,  // crclr 31
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 5\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] Killing instruction 4\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: SET_ALIAS  a3, r3\n"
    "    4: NOP\n"
    "    5: NOP\n"
    "    6: GOTO_IF_Z  r3, L1\n"
    "    7: GET_ALIAS  r5, a5\n"
    "    8: ANDI       r6, r5, -4\n"
    "    9: LOAD_IMM   r4, 0\n"
    "   10: SET_ALIAS  a4, r4\n"
    "   11: SET_ALIAS  a1, r6\n"
    "   12: GOTO       L2\n"
    "   13: LABEL      L1\n"
    "   14: LOAD_IMM   r7, 12\n"
    "   15: SET_ALIAS  a1, r7\n"
    "   16: LOAD_IMM   r8, 0\n"
    "   17: SET_ALIAS  a4, r8\n"
    "   18: LOAD_IMM   r9, 16\n"
    "   19: SET_ALIAS  a1, r9\n"
    "   20: LABEL      L2\n"
    "   21: GET_ALIAS  r10, a2\n"
    "   22: ANDI       r11, r10, -4\n"
    "   23: GET_ALIAS  r12, a3\n"
    "   24: SLLI       r13, r12, 1\n"
    "   25: OR         r14, r11, r13\n"
    "   26: GET_ALIAS  r15, a4\n"
    "   27: OR         r16, r14, r15\n"
    "   28: SET_ALIAS  a2, r16\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,2\n"
    "Block 1: 0 --> [7,12] --> 3\n"
    "Block 2: 0 --> [13,19] --> 3\n"
    "Block 3: 2,1 --> [20,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
