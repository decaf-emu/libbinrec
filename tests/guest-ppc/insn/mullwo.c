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
    0x7C,0x64,0x2D,0xD6,  // mullwo r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: MUL        r4, r2, r3\n"
    "    5: GET_ALIAS  r5, a5\n"
    "    6: ANDI       r6, r5, -1073741825\n"
    "    7: MULHU      r7, r2, r3\n"
    "    8: LOAD_IMM   r8, 0xC0000000\n"
    "    9: SELECT     r9, r8, r7, r7\n"
    "   10: OR         r10, r6, r9\n"
    "   11: SET_ALIAS  a2, r4\n"
    "   12: SET_ALIAS  a5, r10\n"
    "   13: LOAD_IMM   r11, 4\n"
    "   14: SET_ALIAS  a1, r11\n"
    "   15: LABEL      L2\n"
    "   16: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,14] --> 2\n"
    "Block 2: 1 --> [15,16] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
