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
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: SLTSI      r4, r3, 4660\n"
    "    5: SGTSI      r5, r3, 4660\n"
    "    6: SEQI       r6, r3, 4660\n"
    "    7: GET_ALIAS  r7, a8\n"
    "    8: BFEXT      r8, r7, 31, 1\n"
    "    9: SET_ALIAS  a3, r4\n"
    "   10: SET_ALIAS  a4, r5\n"
    "   11: SET_ALIAS  a5, r6\n"
    "   12: SET_ALIAS  a6, r8\n"
    "   13: GOTO_IF_NZ r6, L3\n"
    "   14: LOAD_IMM   r9, 8\n"
    "   15: SET_ALIAS  a1, r9\n"
    "   16: LABEL      L2\n"
    "   17: GOTO       L2\n"
    "   18: LOAD_IMM   r10, 12\n"
    "   19: SET_ALIAS  a1, r10\n"
    "   20: LABEL      L3\n"
    "   21: LOAD_IMM   r11, 16\n"
    "   22: SET_ALIAS  a1, r11\n"
    "   23: LABEL      L4\n"
    "   24: GET_ALIAS  r12, a7\n"
    "   25: ANDI       r13, r12, -16\n"
    "   26: GET_ALIAS  r14, a3\n"
    "   27: SLLI       r15, r14, 3\n"
    "   28: OR         r16, r13, r15\n"
    "   29: GET_ALIAS  r17, a4\n"
    "   30: SLLI       r18, r17, 2\n"
    "   31: OR         r19, r16, r18\n"
    "   32: GET_ALIAS  r20, a5\n"
    "   33: SLLI       r21, r20, 1\n"
    "   34: OR         r22, r19, r21\n"
    "   35: GET_ALIAS  r23, a6\n"
    "   36: OR         r24, r22, r23\n"
    "   37: SET_ALIAS  a7, r24\n"
    "   38: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 928(r1)\n"
    "Alias 8: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,13] --> 2,5\n"
    "Block 2: 1 --> [14,15] --> 3\n"
    "Block 3: 2,3 --> [16,17] --> 3\n"
    "Block 4: <none> --> [18,19] --> 5\n"
    "Block 5: 4,1 --> [20,22] --> 6\n"
    "Block 6: 5 --> [23,38] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
