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
    0xFE,0xE0,0x00,0x4C,  // mtfsb1 23
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: ORI        r6, r5, -1610612480\n"
    "    7: SRLI       r7, r6, 25\n"
    "    8: SRLI       r8, r6, 3\n"
    "    9: AND        r9, r7, r8\n"
    "   10: ANDI       r10, r9, 31\n"
    "   11: SGTUI      r11, r10, 0\n"
    "   12: SLLI       r12, r11, 30\n"
    "   13: OR         r13, r6, r12\n"
    "   14: SET_ALIAS  a2, r13\n"
    "   15: LOAD_IMM   r14, 4\n"
    "   16: SET_ALIAS  a1, r14\n"
    "   17: GET_ALIAS  r15, a2\n"
    "   18: GET_ALIAS  r16, a3\n"
    "   19: ANDI       r17, r15, -522241\n"
    "   20: SLLI       r18, r16, 12\n"
    "   21: OR         r19, r17, r18\n"
    "   22: SET_ALIAS  a2, r19\n"
    "   23: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 944(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,23] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
