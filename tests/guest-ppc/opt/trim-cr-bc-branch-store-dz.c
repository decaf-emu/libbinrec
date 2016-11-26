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
    0x41,0x5E,0x00,0x08,  // bc 10,30,0x10
    0x4F,0xDE,0xF1,0x82,  // crclr 30
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x13\n"
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
    "    6: GET_ALIAS  r5, a5\n"
    "    7: ADDI       r6, r5, -1\n"
    "    8: SET_ALIAS  a5, r6\n"
    "    9: GOTO_IF_NZ r6, L2\n"
    "   10: GOTO_IF_Z  r3, L2\n"
    "   11: SET_ALIAS  a3, r3\n"
    "   12: GOTO       L1\n"
    "   13: LABEL      L2\n"
    "   14: LOAD_IMM   r7, 12\n"
    "   15: SET_ALIAS  a1, r7\n"
    "   16: LOAD_IMM   r8, 0\n"
    "   17: SET_ALIAS  a3, r8\n"
    "   18: LOAD_IMM   r9, 16\n"
    "   19: SET_ALIAS  a1, r9\n"
    "   20: LABEL      L1\n"
    "   21: LOAD_IMM   r10, 20\n"
    "   22: SET_ALIAS  a1, r10\n"
    "   23: GET_ALIAS  r11, a2\n"
    "   24: ANDI       r12, r11, -4\n"
    "   25: GET_ALIAS  r13, a3\n"
    "   26: SLLI       r14, r13, 1\n"
    "   27: OR         r15, r12, r14\n"
    "   28: GET_ALIAS  r16, a4\n"
    "   29: OR         r17, r15, r16\n"
    "   30: SET_ALIAS  a2, r17\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,3\n"
    "Block 1: 0 --> [10,10] --> 2,3\n"
    "Block 2: 1 --> [11,12] --> 4\n"
    "Block 3: 0,1 --> [13,19] --> 4\n"
    "Block 4: 3,2 --> [20,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
