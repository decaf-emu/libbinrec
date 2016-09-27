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
    0x2F,0x80,0xAB,0xCD,  // cmpi cr7,r0,-21555
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a2\n"
    "    3: SLTSI      r3, r2, -21555\n"
    "    4: SGTSI      r4, r2, -21555\n"
    "    5: SEQI       r5, r2, -21555\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: BFEXT      r6, r7, 31, 1\n"
    "    8: SLLI       r8, r5, 1\n"
    "    9: OR         r9, r6, r8\n"
    "   10: SLLI       r10, r4, 2\n"
    "   11: OR         r11, r9, r10\n"
    "   12: SLLI       r12, r3, 3\n"
    "   13: OR         r13, r11, r12\n"
    "   14: SET_ALIAS  a3, r13\n"
    "   15: LOAD_IMM   r14, 4\n"
    "   16: SET_ALIAS  a1, r14\n"
    "   17: LABEL      L2\n"
    "   18: LOAD       r15, 928(r1)\n"
    "   19: ANDI       r16, r15, -16\n"
    "   20: GET_ALIAS  r17, a3\n"
    "   21: OR         r18, r16, r17\n"
    "   22: STORE      928(r1), r18\n"
    "   23: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,16] --> 2\n"
    "Block 2: 1 --> [17,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
