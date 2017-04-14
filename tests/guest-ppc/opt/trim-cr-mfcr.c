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
    0x48,0x00,0x00,0x04,  // b 0xC
    0x7C,0x60,0x00,0x26,  // mfcr r3
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES
                                    | BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x13\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: SET_ALIAS  a4, r3\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a5, r4\n"
    "    6: GOTO       L1\n"
    "    7: LOAD_IMM   r5, 12\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L1\n"
    "   10: GET_ALIAS  r6, a3\n"
    "   11: ANDI       r7, r6, -4\n"
    "   12: GET_ALIAS  r8, a4\n"
    "   13: SLLI       r9, r8, 1\n"
    "   14: OR         r10, r7, r9\n"
    "   15: GET_ALIAS  r11, a5\n"
    "   16: OR         r12, r10, r11\n"
    "   17: SET_ALIAS  a3, r12\n"
    "   18: SET_ALIAS  a2, r12\n"
    "   19: LOAD_IMM   r13, 0\n"
    "   20: SET_ALIAS  a4, r13\n"
    "   21: LOAD_IMM   r14, 20\n"
    "   22: SET_ALIAS  a1, r14\n"
    "   23: GET_ALIAS  r15, a3\n"
    "   24: ANDI       r16, r15, -4\n"
    "   25: GET_ALIAS  r17, a4\n"
    "   26: SLLI       r18, r17, 1\n"
    "   27: OR         r19, r16, r18\n"
    "   28: GET_ALIAS  r20, a5\n"
    "   29: OR         r21, r19, r20\n"
    "   30: SET_ALIAS  a3, r21\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 2\n"
    "Block 1: <none> --> [7,8] --> 2\n"
    "Block 2: 1,0 --> [9,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
