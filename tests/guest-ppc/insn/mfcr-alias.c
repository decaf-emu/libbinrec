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
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: BFEXT      r4, r3, 0, 4\n"
    "    5: SET_ALIAS  a4, r4\n"
    "    6: LOAD       r5, 928(r1)\n"
    "    7: ANDI       r6, r5, -16\n"
    "    8: GET_ALIAS  r7, a4\n"
    "    9: OR         r8, r6, r7\n"
    "   10: SET_ALIAS  a2, r8\n"
    "   11: LOAD_IMM   r9, 8\n"
    "   12: SET_ALIAS  a1, r9\n"
    "   13: LABEL      L2\n"
    "   14: LOAD       r10, 928(r1)\n"
    "   15: ANDI       r11, r10, -16\n"
    "   16: GET_ALIAS  r12, a4\n"
    "   17: OR         r13, r11, r12\n"
    "   18: STORE      928(r1), r13\n"
    "   19: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,12] --> 2\n"
    "Block 2: 1 --> [13,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
