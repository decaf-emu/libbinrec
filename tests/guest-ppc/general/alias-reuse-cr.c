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
    0x2F,0x80,0x12,0x34,  // cmpi cr7,r0,4660
    0x41,0x9E,0x00,0x08,  // beq cr7,0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: SLTSI      r3, r2, 4660\n"
    "    4: SGTSI      r4, r2, 4660\n"
    "    5: SEQI       r5, r2, 4660\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: BFEXT      r6, r7, 31, 1\n"
    "    8: SLLI       r8, r5, 1\n"
    "    9: OR         r9, r6, r8\n"
    "   10: SLLI       r10, r4, 2\n"
    "   11: OR         r11, r9, r10\n"
    "   12: SLLI       r12, r3, 3\n"
    "   13: OR         r13, r11, r12\n"
    "   14: ANDI       r14, r13, 2\n"
    "   15: SET_ALIAS  a3, r13\n"
    "   16: GOTO_IF_NZ r14, L3\n"
    "   17: SET_ALIAS  a3, r13\n"
    "   18: LOAD_IMM   r15, 8\n"
    "   19: SET_ALIAS  a1, r15\n"
    "   20: LABEL      L2\n"
    "   21: GOTO       L2\n"
    "   22: LOAD_IMM   r16, 12\n"
    "   23: SET_ALIAS  a1, r16\n"
    "   24: LABEL      L3\n"
    "   25: LOAD_IMM   r17, 16\n"
    "   26: SET_ALIAS  a1, r17\n"
    "   27: LABEL      L4\n"
    "   28: LOAD       r18, 928(r1)\n"
    "   29: ANDI       r19, r18, -16\n"
    "   30: GET_ALIAS  r20, a3\n"
    "   31: OR         r21, r19, r20\n"
    "   32: STORE      928(r1), r21\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,16] --> 2,5\n"
    "Block 2: 1 --> [17,19] --> 3\n"
    "Block 3: 2,3 --> [20,21] --> 3\n"
    "Block 4: <none> --> [22,23] --> 5\n"
    "Block 5: 4,1 --> [24,26] --> 6\n"
    "Block 6: 5 --> [27,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
