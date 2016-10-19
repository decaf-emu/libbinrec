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
    0x7C,0x64,0x00,0xD1,  // neg. r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: NEG        r4, r3\n"
    "    4: SLTSI      r5, r4, 0\n"
    "    5: SGTSI      r6, r4, 0\n"
    "    6: SEQI       r7, r4, 0\n"
    "    7: GET_ALIAS  r8, a9\n"
    "    8: BFEXT      r9, r8, 31, 1\n"
    "    9: SET_ALIAS  a2, r4\n"
    "   10: SET_ALIAS  a4, r5\n"
    "   11: SET_ALIAS  a5, r6\n"
    "   12: SET_ALIAS  a6, r7\n"
    "   13: SET_ALIAS  a7, r9\n"
    "   14: LOAD_IMM   r10, 4\n"
    "   15: SET_ALIAS  a1, r10\n"
    "   16: GET_ALIAS  r11, a8\n"
    "   17: ANDI       r12, r11, 268435455\n"
    "   18: GET_ALIAS  r13, a4\n"
    "   19: SLLI       r14, r13, 31\n"
    "   20: OR         r15, r12, r14\n"
    "   21: GET_ALIAS  r16, a5\n"
    "   22: SLLI       r17, r16, 30\n"
    "   23: OR         r18, r15, r17\n"
    "   24: GET_ALIAS  r19, a6\n"
    "   25: SLLI       r20, r19, 29\n"
    "   26: OR         r21, r18, r20\n"
    "   27: GET_ALIAS  r22, a7\n"
    "   28: SLLI       r23, r22, 28\n"
    "   29: OR         r24, r21, r23\n"
    "   30: SET_ALIAS  a8, r24\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
