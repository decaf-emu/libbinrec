/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define PRE_INSN_CALLBACK  ((void *)1)

static const uint8_t input[] = {
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
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
        "[info] Killing instruction 6\n"
        "[info] r5 no longer used, setting death = birth\n"
        "[info] Killing instruction 5\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 0x1\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: CALL_TRANSPARENT @r3, r1, r4\n"
    "    5: NOP\n"
    "    6: NOP\n"
    "    7: LOAD_IMM   r6, 0x1\n"
    "    8: LOAD_IMM   r7, 4\n"
    "    9: CALL_TRANSPARENT @r6, r1, r7\n"
    "   10: LOAD_IMM   r8, 0\n"
    "   11: SET_ALIAS  a4, r8\n"
    "   12: LOAD_IMM   r9, 0x1\n"
    "   13: LOAD_IMM   r10, 8\n"
    "   14: CALL_TRANSPARENT @r9, r1, r10\n"
    "   15: GOTO       L1\n"
    "   16: LOAD_IMM   r11, 12\n"
    "   17: SET_ALIAS  a1, r11\n"
    "   18: LABEL      L1\n"
    "   19: LOAD_IMM   r12, 0x1\n"
    "   20: LOAD_IMM   r13, 12\n"
    "   21: CALL_TRANSPARENT @r12, r1, r13\n"
    "   22: LOAD_IMM   r14, 0\n"
    "   23: SET_ALIAS  a3, r14\n"
    "   24: LOAD_IMM   r15, 16\n"
    "   25: SET_ALIAS  a1, r15\n"
    "   26: GET_ALIAS  r16, a2\n"
    "   27: ANDI       r17, r16, -4\n"
    "   28: GET_ALIAS  r18, a3\n"
    "   29: SLLI       r19, r18, 1\n"
    "   30: OR         r20, r17, r19\n"
    "   31: GET_ALIAS  r21, a4\n"
    "   32: OR         r22, r20, r21\n"
    "   33: SET_ALIAS  a2, r22\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,15] --> 2\n"
    "Block 1: <none> --> [16,17] --> 2\n"
    "Block 2: 1,0 --> [18,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
