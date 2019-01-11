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
    0x7C,0x64,0x00,0x34,  // cntlzw r4,r3
    0x54,0x83,0xD9,0x7F,  // rlwinm. r3,r4,27,5,31 (srwi. r3,r4,5)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: CLZ        r4, r3\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: SEQI       r5, r3, 0\n"
    "    6: SET_ALIAS  a2, r5\n"
    "    7: LOAD_IMM   r6, 0\n"
    "    8: XORI       r7, r5, 1\n"
    "    9: GET_ALIAS  r8, a5\n"
    "   10: BFEXT      r9, r8, 31, 1\n"
    "   11: GET_ALIAS  r10, a4\n"
    "   12: SLLI       r11, r6, 3\n"
    "   13: SLLI       r12, r5, 2\n"
    "   14: SLLI       r13, r7, 1\n"
    "   15: OR         r14, r11, r12\n"
    "   16: OR         r15, r13, r9\n"
    "   17: OR         r16, r14, r15\n"
    "   18: BFINS      r17, r10, r16, 28, 4\n"
    "   19: SET_ALIAS  a4, r17\n"
    "   20: LOAD_IMM   r18, 8\n"
    "   21: SET_ALIAS  a1, r18\n"
    "   22: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
