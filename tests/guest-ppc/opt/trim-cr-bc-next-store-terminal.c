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
    0x48,0x00,0x00,0x0C,  // b 0xC
    0x4F,0xDE,0xF1,0x82,  // crclr 30
    0x4E,0x80,0x00,0x20,  // blr
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
    0x41,0x9E,0xFF,0xF0,  // beq cr7,0x4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x17\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 16\n"
        "[info] r8 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GOTO       L2\n"
    "    3: LOAD_IMM   r3, 4\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LABEL      L1\n"
    "    6: LOAD_IMM   r4, 0\n"
    "    7: SET_ALIAS  a3, r4\n"
    "    8: GET_ALIAS  r5, a5\n"
    "    9: ANDI       r6, r5, -4\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: GOTO       L3\n"
    "   12: LOAD_IMM   r7, 12\n"
    "   13: SET_ALIAS  a1, r7\n"
    "   14: LABEL      L2\n"
    "   15: LOAD_IMM   r8, 1\n"
    "   16: NOP\n"
    "   17: LOAD_IMM   r9, 0\n"
    "   18: SET_ALIAS  a4, r9\n"
    "   19: GOTO_IF_NZ r8, L1\n"
    "   20: SET_ALIAS  a3, r8\n"
    "   21: LOAD_IMM   r10, 24\n"
    "   22: SET_ALIAS  a1, r10\n"
    "   23: LABEL      L3\n"
    "   24: GET_ALIAS  r11, a2\n"
    "   25: ANDI       r12, r11, -4\n"
    "   26: GET_ALIAS  r13, a3\n"
    "   27: SLLI       r14, r13, 1\n"
    "   28: OR         r15, r12, r14\n"
    "   29: GET_ALIAS  r16, a4\n"
    "   30: OR         r17, r15, r16\n"
    "   31: SET_ALIAS  a2, r17\n"
    "   32: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,2] --> 4\n"
    "Block 1: <none> --> [3,4] --> 2\n"
    "Block 2: 1,4 --> [5,11] --> 6\n"
    "Block 3: <none> --> [12,13] --> 4\n"
    "Block 4: 3,0 --> [14,19] --> 5,2\n"
    "Block 5: 4 --> [20,22] --> 6\n"
    "Block 6: 5,2 --> [23,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
