/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define BRANCH_EXIT_TEST

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
    "[warning] Skipping TRIM_CR_STORES optimization because branch exit test is enabled\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 0, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: LOAD_IMM   r6, 1\n"
    "    8: SET_ALIAS  a3, r6\n"
    "    9: LOAD_IMM   r7, 0\n"
    "   10: SET_ALIAS  a4, r7\n"
    "   11: LOAD       r8, 1000(r1)\n"
    "   12: GOTO_IF_Z  r8, L1\n"
    "   13: LOAD_IMM   r9, 12\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: GOTO       L2\n"
    "   16: LOAD_IMM   r10, 12\n"
    "   17: SET_ALIAS  a1, r10\n"
    "   18: LABEL      L1\n"
    "   19: LOAD_IMM   r11, 0\n"
    "   20: SET_ALIAS  a3, r11\n"
    "   21: LOAD_IMM   r12, 16\n"
    "   22: SET_ALIAS  a1, r12\n"
    "   23: LABEL      L2\n"
    "   24: GET_ALIAS  r13, a2\n"
    "   25: ANDI       r14, r13, -4\n"
    "   26: GET_ALIAS  r15, a3\n"
    "   27: SLLI       r16, r15, 1\n"
    "   28: OR         r17, r14, r16\n"
    "   29: GET_ALIAS  r18, a4\n"
    "   30: OR         r19, r17, r18\n"
    "   31: SET_ALIAS  a2, r19\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,3\n"
    "Block 1: 0 --> [13,15] --> 4\n"
    "Block 2: <none> --> [16,17] --> 3\n"
    "Block 3: 2,0 --> [18,22] --> 4\n"
    "Block 4: 3,1 --> [23,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"