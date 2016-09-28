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
    0x30,0x60,0x12,0x34,  // addic r3,r0,4660
    0x7F,0x80,0x04,0x00,  // mcrxr cr7
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: ADDI       r3, r2, 4660\n"
    "    4: GET_ALIAS  r4, a5\n"
    "    5: SLTUI      r5, r3, 4660\n"
    "    6: BFINS      r6, r4, r5, 29, 1\n"
    "    7: SRLI       r7, r6, 28\n"
    "    8: ANDI       r8, r6, 268435455\n"
    "    9: SET_ALIAS  a3, r3\n"
    "   10: SET_ALIAS  a4, r7\n"
    "   11: SET_ALIAS  a5, r8\n"
    "   12: LOAD_IMM   r9, 8\n"
    "   13: SET_ALIAS  a1, r9\n"
    "   14: LABEL      L2\n"
    "   15: LOAD       r10, 928(r1)\n"
    "   16: ANDI       r11, r10, -16\n"
    "   17: GET_ALIAS  r12, a4\n"
    "   18: OR         r13, r11, r12\n"
    "   19: STORE      928(r1), r13\n"
    "   20: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,13] --> 2\n"
    "Block 2: 1 --> [14,20] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
