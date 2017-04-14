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
    0x54,0x83,0x29,0x8F,  // rlwinm. r3,r4,5,6,7
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: RORI       r4, r3, 27\n"
    "    4: ANDI       r5, r4, 50331648\n"
    "    5: SET_ALIAS  a2, r5\n"
    "    6: SLTSI      r6, r5, 0\n"
    "    7: SGTSI      r7, r5, 0\n"
    "    8: SEQI       r8, r5, 0\n"
    "    9: GET_ALIAS  r9, a5\n"
    "   10: BFEXT      r10, r9, 31, 1\n"
    "   11: GET_ALIAS  r11, a4\n"
    "   12: SLLI       r12, r6, 3\n"
    "   13: SLLI       r13, r7, 2\n"
    "   14: SLLI       r14, r8, 1\n"
    "   15: OR         r15, r12, r13\n"
    "   16: OR         r16, r14, r10\n"
    "   17: OR         r17, r15, r16\n"
    "   18: BFINS      r18, r11, r17, 28, 4\n"
    "   19: SET_ALIAS  a4, r18\n"
    "   20: LOAD_IMM   r19, 4\n"
    "   21: SET_ALIAS  a1, r19\n"
    "   22: RETURN\n"
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
