/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define CHAINING

static const uint8_t input[] = {
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x48,0x00,0x00,0x09,  // bl 0xC
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 1, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: LOAD_IMM   r5, 1\n"
    "    6: SET_ALIAS  a3, r5\n"
    "    7: LOAD_IMM   r6, 12\n"
    "    8: LOAD_IMM   r7, 8\n"
    "    9: SET_ALIAS  a4, r7\n"
    "   10: SET_ALIAS  a1, r6\n"
    "   11: GET_ALIAS  r8, a2\n"
    "   12: ANDI       r9, r8, -3\n"
    "   13: SLLI       r10, r5, 1\n"
    "   14: OR         r11, r9, r10\n"
    "   15: SET_ALIAS  a2, r11\n"
    "   16: CHAIN      r1, r2\n"
    "   17: LOAD       r12, 1000(r1)\n"
    "   18: CALL       r13, @r12, r1, r6\n"
    "   19: CHAIN_RESOLVE @16, r13\n"
    "   20: RETURN     r1\n"
    "   21: LOAD_IMM   r14, 8\n"
    "   22: SET_ALIAS  a1, r14\n"
    "   23: GET_ALIAS  r15, a2\n"
    "   24: ANDI       r16, r15, -3\n"
    "   25: GET_ALIAS  r17, a3\n"
    "   26: SLLI       r18, r17, 1\n"
    "   27: OR         r19, r16, r18\n"
    "   28: SET_ALIAS  a2, r19\n"
    "   29: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,20] --> <none>\n"
    "Block 1: <none> --> [21,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
