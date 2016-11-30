/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define BRANCH_CALLBACK

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
    "[warning] Skipping TRIM_CR_STORES optimization because branch callback is enabled\n"
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
    "   11: LOAD_IMM   r8, 12\n"
    "   12: SET_ALIAS  a1, r8\n"
    "   13: LOAD       r9, 1000(r1)\n"
    "   14: LOAD_IMM   r10, 8\n"
    "   15: CALL       r11, @r9, r1, r10\n"
    "   16: GOTO_IF_Z  r11, L2\n"
    "   17: GOTO       L1\n"
    "   18: LOAD_IMM   r12, 12\n"
    "   19: SET_ALIAS  a1, r12\n"
    "   20: LABEL      L1\n"
    "   21: LOAD_IMM   r13, 0\n"
    "   22: SET_ALIAS  a3, r13\n"
    "   23: LOAD_IMM   r14, 16\n"
    "   24: SET_ALIAS  a1, r14\n"
    "   25: LABEL      L2\n"
    "   26: GET_ALIAS  r15, a2\n"
    "   27: ANDI       r16, r15, -4\n"
    "   28: GET_ALIAS  r17, a3\n"
    "   29: SLLI       r18, r17, 1\n"
    "   30: OR         r19, r16, r18\n"
    "   31: GET_ALIAS  r20, a4\n"
    "   32: OR         r21, r19, r20\n"
    "   33: SET_ALIAS  a2, r21\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,16] --> 1,4\n"
    "Block 1: 0 --> [17,17] --> 3\n"
    "Block 2: <none> --> [18,19] --> 3\n"
    "Block 3: 2,1 --> [20,24] --> 4\n"
    "Block 4: 3,0 --> [25,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
