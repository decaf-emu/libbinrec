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
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
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
    "   12: LOAD_IMM   r9, 4\n"
    "   13: SET_ALIAS  a1, r9\n"
    "   14: GET_ALIAS  r10, a7\n"
    "   15: ANDI       r11, r10, -16\n"
    "   16: GET_ALIAS  r12, a3\n"
    "   17: SLLI       r13, r12, 3\n"
    "   18: OR         r14, r11, r13\n"
    "   19: GET_ALIAS  r15, a4\n"
    "   20: SLLI       r16, r15, 2\n"
    "   21: OR         r17, r14, r16\n"
    "   22: GET_ALIAS  r18, a5\n"
    "   23: SLLI       r19, r18, 1\n"
    "   24: OR         r20, r17, r19\n"
    "   25: GET_ALIAS  r21, a6\n"
    "   26: OR         r22, r20, r21\n"
    "   27: SET_ALIAS  a7, r22\n"
    "   28: RETURN\n"
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
    "Block 0: <none> --> [0,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
