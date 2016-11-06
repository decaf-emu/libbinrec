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
    0x7F,0x80,0x04,0x00,  // mcrxr cr7
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: BFEXT      r5, r3, 30, 1\n"
    "    5: BFEXT      r6, r3, 29, 1\n"
    "    6: BFEXT      r7, r3, 28, 1\n"
    "    7: SET_ALIAS  a2, r4\n"
    "    8: SET_ALIAS  a3, r5\n"
    "    9: SET_ALIAS  a4, r6\n"
    "   10: SET_ALIAS  a5, r7\n"
    "   11: ANDI       r8, r3, 268435455\n"
    "   12: SET_ALIAS  a7, r8\n"
    "   13: LOAD_IMM   r9, 4\n"
    "   14: SET_ALIAS  a1, r9\n"
    "   15: GET_ALIAS  r10, a6\n"
    "   16: ANDI       r11, r10, -16\n"
    "   17: GET_ALIAS  r12, a2\n"
    "   18: SLLI       r13, r12, 3\n"
    "   19: OR         r14, r11, r13\n"
    "   20: GET_ALIAS  r15, a3\n"
    "   21: SLLI       r16, r15, 2\n"
    "   22: OR         r17, r14, r16\n"
    "   23: GET_ALIAS  r18, a4\n"
    "   24: SLLI       r19, r18, 1\n"
    "   25: OR         r20, r17, r19\n"
    "   26: GET_ALIAS  r21, a5\n"
    "   27: OR         r22, r20, r21\n"
    "   28: SET_ALIAS  a6, r22\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
