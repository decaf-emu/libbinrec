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

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: SLTSI      r4, r3, 4660\n"
    "    4: SGTSI      r5, r3, 4660\n"
    "    5: SEQI       r6, r3, 4660\n"
    "    6: GET_ALIAS  r7, a8\n"
    "    7: BFEXT      r8, r7, 31, 1\n"
    "    8: SET_ALIAS  a3, r4\n"
    "    9: SET_ALIAS  a4, r5\n"
    "   10: SET_ALIAS  a5, r6\n"
    "   11: SET_ALIAS  a6, r8\n"
    "   12: GOTO_IF_NZ r6, L2\n"
    "   13: LOAD_IMM   r9, 8\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: LABEL      L1\n"
    "   16: GOTO       L1\n"
    "   17: LOAD_IMM   r10, 12\n"
    "   18: SET_ALIAS  a1, r10\n"
    "   19: LABEL      L2\n"
    "   20: LOAD_IMM   r11, 16\n"
    "   21: SET_ALIAS  a1, r11\n"
    "   22: GET_ALIAS  r12, a7\n"
    "   23: ANDI       r13, r12, -16\n"
    "   24: GET_ALIAS  r14, a3\n"
    "   25: SLLI       r15, r14, 3\n"
    "   26: OR         r16, r13, r15\n"
    "   27: GET_ALIAS  r17, a4\n"
    "   28: SLLI       r18, r17, 2\n"
    "   29: OR         r19, r16, r18\n"
    "   30: GET_ALIAS  r20, a5\n"
    "   31: SLLI       r21, r20, 1\n"
    "   32: OR         r22, r19, r21\n"
    "   33: GET_ALIAS  r23, a6\n"
    "   34: OR         r24, r22, r23\n"
    "   35: SET_ALIAS  a7, r24\n"
    "   36: RETURN\n"
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
    "Block 0: <none> --> [0,12] --> 1,4\n"
    "Block 1: 0 --> [13,14] --> 2\n"
    "Block 2: 1,2 --> [15,16] --> 2\n"
    "Block 3: <none> --> [17,18] --> 4\n"
    "Block 4: 3,0 --> [19,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
