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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 0, 4\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: BFEXT      r6, r5, 0, 4\n"
    "    8: LOAD       r7, 928(r1)\n"
    "    9: ANDI       r8, r7, -16\n"
    "   10: OR         r9, r8, r6\n"
    "   11: SET_ALIAS  a2, r9\n"
    "   12: SET_ALIAS  a4, r6\n"
    "   13: LOAD_IMM   r10, 8\n"
    "   14: SET_ALIAS  a1, r10\n"
    "   15: LABEL      L2\n"
    "   16: LOAD       r11, 928(r1)\n"
    "   17: ANDI       r12, r11, -16\n"
    "   18: GET_ALIAS  r13, a4\n"
    "   19: OR         r14, r12, r13\n"
    "   20: STORE      928(r1), r14\n"
    "   21: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,14] --> 2\n"
    "Block 2: 1 --> [15,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
