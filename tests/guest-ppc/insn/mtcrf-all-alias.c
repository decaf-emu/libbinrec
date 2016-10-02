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
    0x70,0x64,0x00,0xFF,  // andi. r4,r3,255
    0x7C,0x6F,0xF1,0x20,  // mtcrf r3
    0x41,0x82,0x00,0x08,  // beq 0x10
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: ANDI       r4, r3, 255\n"
    "    5: SLTSI      r5, r4, 0\n"
    "    6: SGTSI      r6, r4, 0\n"
    "    7: SEQI       r7, r4, 0\n"
    "    8: GET_ALIAS  r9, a5\n"
    "    9: BFEXT      r8, r9, 31, 1\n"
    "   10: SLLI       r10, r7, 1\n"
    "   11: OR         r11, r8, r10\n"
    "   12: SLLI       r12, r6, 2\n"
    "   13: OR         r13, r11, r12\n"
    "   14: SLLI       r14, r5, 3\n"
    "   15: OR         r15, r13, r14\n"
    "   16: STORE      928(r1), r3\n"
    "   17: BFEXT      r16, r3, 28, 4\n"
    "   18: LOAD_IMM   r17, 16\n"
    "   19: SET_ALIAS  a3, r4\n"
    "   20: SET_ALIAS  a4, r16\n"
    "   21: ANDI       r18, r16, 2\n"
    "   22: GOTO_IF_Z  r18, L3\n"
    "   23: SET_ALIAS  a1, r17\n"
    "   24: GOTO       L2\n"
    "   25: LABEL      L3\n"
    "   26: LOAD_IMM   r19, 12\n"
    "   27: SET_ALIAS  a1, r19\n"
    "   28: LABEL      L2\n"
    "   29: LOAD       r20, 928(r1)\n"
    "   30: ANDI       r21, r20, 268435455\n"
    "   31: GET_ALIAS  r22, a4\n"
    "   32: SLLI       r23, r22, 28\n"
    "   33: OR         r24, r21, r23\n"
    "   34: STORE      928(r1), r24\n"
    "   35: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,22] --> 2,3\n"
    "Block 2: 1 --> [23,24] --> 4\n"
    "Block 3: 1 --> [25,27] --> 4\n"
    "Block 4: 3,2 --> [28,35] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
