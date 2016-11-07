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
    0xFF,0x98,0x00,0x80,  // mcrfs cr7,cr6
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: BFEXT      r4, r3, 7, 1\n"
    "    4: BFEXT      r5, r3, 6, 1\n"
    "    5: BFEXT      r6, r3, 5, 1\n"
    "    6: BFEXT      r7, r3, 4, 1\n"
    "    7: SET_ALIAS  a2, r4\n"
    "    8: SET_ALIAS  a3, r5\n"
    "    9: SET_ALIAS  a4, r6\n"
    "   10: SET_ALIAS  a5, r7\n"
    "   11: LOAD_IMM   r8, 4\n"
    "   12: SET_ALIAS  a1, r8\n"
    "   13: GET_ALIAS  r9, a6\n"
    "   14: ANDI       r10, r9, -16\n"
    "   15: GET_ALIAS  r11, a2\n"
    "   16: SLLI       r12, r11, 3\n"
    "   17: OR         r13, r10, r12\n"
    "   18: GET_ALIAS  r14, a3\n"
    "   19: SLLI       r15, r14, 2\n"
    "   20: OR         r16, r13, r15\n"
    "   21: GET_ALIAS  r17, a4\n"
    "   22: SLLI       r18, r17, 1\n"
    "   23: OR         r19, r16, r18\n"
    "   24: GET_ALIAS  r20, a5\n"
    "   25: OR         r21, r19, r20\n"
    "   26: SET_ALIAS  a6, r21\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
