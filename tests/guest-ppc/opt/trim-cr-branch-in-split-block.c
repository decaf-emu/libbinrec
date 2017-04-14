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
    0x41,0x82,0x00,0x0C,  // beq 0xC
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
    0x4F,0xDE,0xF1,0x82,  // crclr 30
    0x41,0x82,0xFF,0xFC,  // beq 0xC
    0x4F,0xDE,0xF2,0x42,  // crset 30
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x17\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 11\n"
        "[info] r8 no longer used, setting death = birth\n"
        "[info] Killing instruction 10\n"
        "[info] Killing instruction 18\n"
        "[info] r11 no longer used, setting death = birth\n"
        "[info] Killing instruction 17\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 0, 1\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: ANDI       r6, r5, 536870912\n"
    "    7: GOTO_IF_NZ r6, L1\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: SET_ALIAS  a1, r7\n"
    "   10: NOP\n"
    "   11: NOP\n"
    "   12: LOAD_IMM   r9, 0\n"
    "   13: SET_ALIAS  a4, r9\n"
    "   14: LOAD_IMM   r10, 12\n"
    "   15: SET_ALIAS  a1, r10\n"
    "   16: LABEL      L1\n"
    "   17: NOP\n"
    "   18: NOP\n"
    "   19: GET_ALIAS  r12, a2\n"
    "   20: ANDI       r13, r12, 536870912\n"
    "   21: GOTO_IF_NZ r13, L1\n"
    "   22: LOAD_IMM   r14, 20\n"
    "   23: SET_ALIAS  a1, r14\n"
    "   24: LOAD_IMM   r15, 1\n"
    "   25: SET_ALIAS  a3, r15\n"
    "   26: LOAD_IMM   r16, 24\n"
    "   27: SET_ALIAS  a1, r16\n"
    "   28: GET_ALIAS  r17, a2\n"
    "   29: ANDI       r18, r17, -4\n"
    "   30: GET_ALIAS  r19, a3\n"
    "   31: SLLI       r20, r19, 1\n"
    "   32: OR         r21, r18, r20\n"
    "   33: GET_ALIAS  r22, a4\n"
    "   34: OR         r23, r21, r22\n"
    "   35: SET_ALIAS  a2, r23\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,2\n"
    "Block 1: 0 --> [8,15] --> 2\n"
    "Block 2: 1,0,2 --> [16,21] --> 3,2\n"
    "Block 3: 2 --> [22,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
