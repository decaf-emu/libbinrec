/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

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
    "    7: GOTO       L1\n"
    "    8: LOAD_IMM   r6, 4\n"
    "    9: SET_ALIAS  a1, r6\n"
    "   10: LOAD_IMM   r7, 0x2\n"
    "   11: LOAD_IMM   r8, 0\n"
    "   12: CALL_TRANSPARENT @r7, r1, r8\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LOAD_IMM   r10, 8\n"
    "   16: SET_ALIAS  a1, r10\n"
    "   17: LOAD_IMM   r11, 0x2\n"
    "   18: LOAD_IMM   r12, 4\n"
    "   19: CALL_TRANSPARENT @r11, r1, r12\n"
    "   20: LOAD_IMM   r13, 8\n"
    "   21: SET_ALIAS  a1, r13\n"
    "   22: LABEL      L1\n"
    "   23: LOAD_IMM   r14, 12\n"
    "   24: SET_ALIAS  a1, r14\n"
    "   25: LOAD_IMM   r15, 0x2\n"
    "   26: LOAD_IMM   r16, 8\n"
    "   27: CALL_TRANSPARENT @r15, r1, r16\n"
    "   28: LOAD_IMM   r17, 12\n"
    "   29: SET_ALIAS  a1, r17\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 2\n"
    "Block 1: <none> --> [8,21] --> 2\n"
    "Block 2: 1,0 --> [22,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
