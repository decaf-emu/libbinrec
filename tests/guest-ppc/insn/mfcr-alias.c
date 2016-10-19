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
    0x7C,0x80,0x11,0x20,  // mtcrf 1,r4
    0x7C,0x60,0x00,0x26,  // mfcr r3
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: BFEXT      r5, r3, 2, 1\n"
    "    5: BFEXT      r6, r3, 1, 1\n"
    "    6: BFEXT      r7, r3, 0, 1\n"
    "    7: GET_ALIAS  r8, a8\n"
    "    8: ANDI       r9, r8, -16\n"
    "    9: SLLI       r10, r4, 3\n"
    "   10: OR         r11, r9, r10\n"
    "   11: SLLI       r12, r5, 2\n"
    "   12: OR         r13, r11, r12\n"
    "   13: SLLI       r14, r6, 1\n"
    "   14: OR         r15, r13, r14\n"
    "   15: OR         r16, r15, r7\n"
    "   16: SET_ALIAS  a2, r16\n"
    "   17: SET_ALIAS  a8, r16\n"
    "   18: LOAD_IMM   r17, 8\n"
    "   19: SET_ALIAS  a1, r17\n"
    "   20: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "\n"
    "Block 0: <none> --> [0,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
