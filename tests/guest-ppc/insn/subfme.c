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
    0x7C,0x64,0x01,0xD0,  // subfme r3,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: NOT        r3, r2\n"
    "    4: ADDI       r4, r3, -1\n"
    "    5: GET_ALIAS  r5, a4\n"
    "    6: BFEXT      r6, r5, 29, 1\n"
    "    7: ADD        r7, r4, r6\n"
    "    8: SRLI       r8, r3, 31\n"
    "    9: SRLI       r9, r7, 31\n"
    "   10: XORI       r10, r9, 1\n"
    "   11: OR         r11, r8, r10\n"
    "   12: BFINS      r12, r5, r11, 29, 1\n"
    "   13: SET_ALIAS  a2, r7\n"
    "   14: SET_ALIAS  a4, r12\n"
    "   15: LOAD_IMM   r13, 4\n"
    "   16: SET_ALIAS  a1, r13\n"
    "   17: LABEL      L2\n"
    "   18: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,16] --> 2\n"
    "Block 2: 1 --> [17,18] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
