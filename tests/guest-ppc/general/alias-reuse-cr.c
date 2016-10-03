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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: SLTSI      r6, r5, 4660\n"
    "    8: SGTSI      r7, r5, 4660\n"
    "    9: SEQI       r8, r5, 4660\n"
    "   10: GET_ALIAS  r10, a4\n"
    "   11: BFEXT      r9, r10, 31, 1\n"
    "   12: SLLI       r11, r8, 1\n"
    "   13: OR         r12, r9, r11\n"
    "   14: SLLI       r13, r7, 2\n"
    "   15: OR         r14, r12, r13\n"
    "   16: SLLI       r15, r6, 3\n"
    "   17: OR         r16, r14, r15\n"
    "   18: SET_ALIAS  a3, r16\n"
    "   19: ANDI       r17, r16, 2\n"
    "   20: GOTO_IF_NZ r17, L3\n"
    "   21: LOAD_IMM   r18, 8\n"
    "   22: SET_ALIAS  a1, r18\n"
    "   23: LABEL      L2\n"
    "   24: GOTO       L2\n"
    "   25: LOAD_IMM   r19, 12\n"
    "   26: SET_ALIAS  a1, r19\n"
    "   27: LABEL      L3\n"
    "   28: LOAD_IMM   r20, 16\n"
    "   29: SET_ALIAS  a1, r20\n"
    "   30: LABEL      L4\n"
    "   31: LOAD       r21, 928(r1)\n"
    "   32: ANDI       r22, r21, -16\n"
    "   33: GET_ALIAS  r23, a3\n"
    "   34: OR         r24, r22, r23\n"
    "   35: STORE      928(r1), r24\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,20] --> 2,5\n"
    "Block 2: 1 --> [21,22] --> 3\n"
    "Block 3: 2,3 --> [23,24] --> 3\n"
    "Block 4: <none> --> [25,26] --> 5\n"
    "Block 5: 4,1 --> [27,29] --> 6\n"
    "Block 6: 5 --> [30,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
