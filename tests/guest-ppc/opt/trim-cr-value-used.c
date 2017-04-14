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
    0x4F,0xFE,0xF3,0x82,  // crmove 31,30
    0x48,0x00,0x00,0x04,  // b 0xC
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
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: NOP\n"
    "    4: OR         r4, r3, r3\n"
    "    5: SET_ALIAS  a4, r4\n"
    "    6: GOTO       L1\n"
    "    7: LOAD_IMM   r5, 12\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L1\n"
    "   10: LOAD_IMM   r6, 0\n"
    "   11: SET_ALIAS  a3, r6\n"
    "   12: LOAD_IMM   r7, 16\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: GET_ALIAS  r8, a2\n"
    "   15: ANDI       r9, r8, -4\n"
    "   16: GET_ALIAS  r10, a3\n"
    "   17: SLLI       r11, r10, 1\n"
    "   18: OR         r12, r9, r11\n"
    "   19: GET_ALIAS  r13, a4\n"
    "   20: OR         r14, r12, r13\n"
    "   21: SET_ALIAS  a2, r14\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 2\n"
    "Block 1: <none> --> [7,8] --> 2\n"
    "Block 2: 1,0 --> [9,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
