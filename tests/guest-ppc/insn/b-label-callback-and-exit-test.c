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
#define POST_INSN_CALLBACK  ((void (*)(void *, uint32_t))2)

static const uint8_t input[] = {
    0x48,0x00,0x00,0x08,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 8\n"
    "    3: SET_ALIAS  a1, r3\n"
    "    4: LOAD_IMM   r4, 0x2\n"
    "    5: LOAD_IMM   r5, 0\n"
    "    6: CALL_TRANSPARENT @r4, r1, r5\n"
    "    7: LOAD       r6, 1000(r1)\n"
    "    8: GOTO_IF_Z  r6, L1\n"
    "    9: LOAD_IMM   r7, 8\n"
    "   10: SET_ALIAS  a1, r7\n"
    "   11: GOTO       L2\n"
    "   12: LOAD_IMM   r8, 4\n"
    "   13: SET_ALIAS  a1, r8\n"
    "   14: LOAD_IMM   r9, 0x2\n"
    "   15: LOAD_IMM   r10, 0\n"
    "   16: CALL_TRANSPARENT @r9, r1, r10\n"
    "   17: LOAD_IMM   r11, 4\n"
    "   18: SET_ALIAS  a1, r11\n"
    "   19: LOAD_IMM   r12, 8\n"
    "   20: SET_ALIAS  a1, r12\n"
    "   21: LOAD_IMM   r13, 0x2\n"
    "   22: LOAD_IMM   r14, 4\n"
    "   23: CALL_TRANSPARENT @r13, r1, r14\n"
    "   24: LOAD_IMM   r15, 8\n"
    "   25: SET_ALIAS  a1, r15\n"
    "   26: LABEL      L1\n"
    "   27: LOAD_IMM   r16, 12\n"
    "   28: SET_ALIAS  a1, r16\n"
    "   29: LOAD_IMM   r17, 0x2\n"
    "   30: LOAD_IMM   r18, 8\n"
    "   31: CALL_TRANSPARENT @r17, r1, r18\n"
    "   32: LOAD_IMM   r19, 12\n"
    "   33: SET_ALIAS  a1, r19\n"
    "   34: LABEL      L2\n"
    "   35: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,3\n"
    "Block 1: 0 --> [9,11] --> 4\n"
    "Block 2: <none> --> [12,25] --> 3\n"
    "Block 3: 2,0 --> [26,33] --> 4\n"
    "Block 4: 3,1 --> [34,35] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
