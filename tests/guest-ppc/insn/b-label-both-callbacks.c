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
#define POST_INSN_CALLBACK  ((void (*)(void *, uint32_t))2)

static const uint8_t input[] = {
    0x48,0x00,0x00,0x08,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_IMM   r3, 8\n"
    "    4: SET_ALIAS  a1, r3\n"
    "    5: LOAD_IMM   r4, 0x2\n"
    "    6: LOAD_IMM   r5, 0\n"
    "    7: CALL_TRANSPARENT @r4, r1, r5\n"
    "    8: LOAD       r6, 992(r1)\n"
    "    9: LOAD_IMM   r7, 0\n"
    "   10: CALL       r8, @r6, r1, r7\n"
    "   11: GOTO_IF_Z  r8, L4\n"
    "   12: GOTO       L3\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LOAD_IMM   r10, 0x2\n"
    "   16: LOAD_IMM   r11, 0\n"
    "   17: CALL_TRANSPARENT @r10, r1, r11\n"
    "   18: LOAD_IMM   r12, 4\n"
    "   19: SET_ALIAS  a1, r12\n"
    "   20: LABEL      L2\n"
    "   21: LOAD_IMM   r13, 8\n"
    "   22: SET_ALIAS  a1, r13\n"
    "   23: LOAD_IMM   r14, 0x2\n"
    "   24: LOAD_IMM   r15, 4\n"
    "   25: CALL_TRANSPARENT @r14, r1, r15\n"
    "   26: LOAD_IMM   r16, 8\n"
    "   27: SET_ALIAS  a1, r16\n"
    "   28: LABEL      L3\n"
    "   29: LOAD_IMM   r17, 12\n"
    "   30: SET_ALIAS  a1, r17\n"
    "   31: LOAD_IMM   r18, 0x2\n"
    "   32: LOAD_IMM   r19, 8\n"
    "   33: CALL_TRANSPARENT @r18, r1, r19\n"
    "   34: LOAD_IMM   r20, 12\n"
    "   35: SET_ALIAS  a1, r20\n"
    "   36: LABEL      L4\n"
    "   37: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,11] --> 2,6\n"
    "Block 2: 1 --> [12,12] --> 5\n"
    "Block 3: <none> --> [13,19] --> 4\n"
    "Block 4: 3 --> [20,27] --> 5\n"
    "Block 5: 4,2 --> [28,35] --> 6\n"
    "Block 6: 5,1 --> [36,37] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
