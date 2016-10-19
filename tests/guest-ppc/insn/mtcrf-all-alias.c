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
    0x7C,0x6F,0xF1,0x20,  // mtcr r3
    0x41,0x82,0x00,0x08,  // beq 0x10
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, 255\n"
    "    4: SLTSI      r5, r4, 0\n"
    "    5: SGTSI      r6, r4, 0\n"
    "    6: SEQI       r7, r4, 0\n"
    "    7: GET_ALIAS  r8, a9\n"
    "    8: BFEXT      r9, r8, 31, 1\n"
    "    9: LOAD_IMM   r10, 16\n"
    "   10: SET_ALIAS  a3, r4\n"
    "   11: SET_ALIAS  a8, r3\n"
    "   12: ANDI       r11, r3, 536870912\n"
    "   13: GOTO_IF_Z  r11, L1\n"
    "   14: SET_ALIAS  a1, r10\n"
    "   15: GOTO       L2\n"
    "   16: LABEL      L1\n"
    "   17: LOAD_IMM   r12, 12\n"
    "   18: SET_ALIAS  a1, r12\n"
    "   19: LABEL      L2\n"
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
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,13] --> 1,2\n"
    "Block 1: 0 --> [14,15] --> 3\n"
    "Block 2: 0 --> [16,18] --> 3\n"
    "Block 3: 2,1 --> [19,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
