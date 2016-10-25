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
    0x7C,0x64,0x29,0xD7,  // mullw. r3,r4,r5
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: MUL        r5, r3, r4\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: SLTSI      r6, r5, 0\n"
    "    7: SGTSI      r7, r5, 0\n"
    "    8: SEQI       r8, r5, 0\n"
    "    9: GET_ALIAS  r9, a10\n"
    "   10: BFEXT      r10, r9, 31, 1\n"
    "   11: SET_ALIAS  a5, r6\n"
    "   12: SET_ALIAS  a6, r7\n"
    "   13: SET_ALIAS  a7, r8\n"
    "   14: SET_ALIAS  a8, r10\n"
    "   15: LOAD_IMM   r11, 4\n"
    "   16: SET_ALIAS  a1, r11\n"
    "   17: GET_ALIAS  r12, a9\n"
    "   18: ANDI       r13, r12, 268435455\n"
    "   19: GET_ALIAS  r14, a5\n"
    "   20: SLLI       r15, r14, 31\n"
    "   21: OR         r16, r13, r15\n"
    "   22: GET_ALIAS  r17, a6\n"
    "   23: SLLI       r18, r17, 30\n"
    "   24: OR         r19, r16, r18\n"
    "   25: GET_ALIAS  r20, a7\n"
    "   26: SLLI       r21, r20, 29\n"
    "   27: OR         r22, r19, r21\n"
    "   28: GET_ALIAS  r23, a8\n"
    "   29: SLLI       r24, r23, 28\n"
    "   30: OR         r25, r22, r24\n"
    "   31: SET_ALIAS  a9, r25\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 928(r1)\n"
    "Alias 10: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
