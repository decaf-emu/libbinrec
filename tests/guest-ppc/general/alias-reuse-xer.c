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
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a2\n"
    "    4: ADDI       r4, r3, 4660\n"
    "    5: GET_ALIAS  r5, a5\n"
    "    6: SLTUI      r6, r4, 4660\n"
    "    7: BFINS      r7, r5, r6, 29, 1\n"
    "    8: SRLI       r8, r7, 28\n"
    "    9: ANDI       r9, r7, 268435455\n"
    "   10: SET_ALIAS  a3, r4\n"
    "   11: SET_ALIAS  a4, r8\n"
    "   12: SET_ALIAS  a5, r9\n"
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
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,14] --> 2\n"
    "Block 2: 1 --> [15,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
