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
    0x70,0x03,0x12,0x34,  // andi. r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: ANDI       r4, r3, 4660\n"
    "    5: SLTSI      r5, r4, 0\n"
    "    6: SGTSI      r6, r4, 0\n"
    "    7: SEQI       r7, r4, 0\n"
    "    8: GET_ALIAS  r8, a9\n"
    "    9: BFEXT      r9, r8, 31, 1\n"
    "   10: SET_ALIAS  a3, r4\n"
    "   11: SET_ALIAS  a4, r5\n"
    "   12: SET_ALIAS  a5, r6\n"
    "   13: SET_ALIAS  a6, r7\n"
    "   14: SET_ALIAS  a7, r9\n"
    "   15: LOAD_IMM   r10, 4\n"
    "   16: SET_ALIAS  a1, r10\n"
    "   17: LABEL      L2\n"
    "   18: GET_ALIAS  r11, a8\n"
    "   19: ANDI       r12, r11, 268435455\n"
    "   20: GET_ALIAS  r13, a4\n"
    "   21: SLLI       r14, r13, 31\n"
    "   22: OR         r15, r12, r14\n"
    "   23: GET_ALIAS  r16, a5\n"
    "   24: SLLI       r17, r16, 30\n"
    "   25: OR         r18, r15, r17\n"
    "   26: GET_ALIAS  r19, a6\n"
    "   27: SLLI       r20, r19, 29\n"
    "   28: OR         r21, r18, r20\n"
    "   29: GET_ALIAS  r22, a7\n"
    "   30: SLLI       r23, r22, 28\n"
    "   31: OR         r24, r21, r23\n"
    "   32: SET_ALIAS  a8, r24\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,16] --> 2\n"
    "Block 2: 1 --> [17,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
