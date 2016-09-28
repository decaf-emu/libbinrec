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
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: BFEXT      r3, r2, 0, 4\n"
    "    4: SET_ALIAS  a4, r3\n"
    "    5: LOAD       r4, 928(r1)\n"
    "    6: ANDI       r5, r4, -16\n"
    "    7: GET_ALIAS  r6, a4\n"
    "    8: OR         r7, r5, r6\n"
    "    9: SET_ALIAS  a2, r7\n"
    "   10: LOAD_IMM   r8, 8\n"
    "   11: SET_ALIAS  a1, r8\n"
    "   12: LABEL      L2\n"
    "   13: LOAD       r9, 928(r1)\n"
    "   14: ANDI       r10, r9, -16\n"
    "   15: GET_ALIAS  r11, a4\n"
    "   16: OR         r12, r10, r11\n"
    "   17: STORE      928(r1), r12\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,11] --> 2\n"
    "Block 2: 1 --> [12,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
