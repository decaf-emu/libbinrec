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
    0x70,0x03,0xAB,0xCD,  // andi. r3,r0,43981
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ANDI       r4, r3, 43981\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: SLTSI      r5, r4, 0\n"
    "    6: SGTSI      r6, r4, 0\n"
    "    7: SEQI       r7, r4, 0\n"
    "    8: GET_ALIAS  r8, a5\n"
    "    9: BFEXT      r9, r8, 31, 1\n"
    "   10: GET_ALIAS  r10, a4\n"
    "   11: SLLI       r11, r5, 3\n"
    "   12: SLLI       r12, r6, 2\n"
    "   13: SLLI       r13, r7, 1\n"
    "   14: OR         r14, r11, r12\n"
    "   15: OR         r15, r13, r9\n"
    "   16: OR         r16, r14, r15\n"
    "   17: BFINS      r17, r10, r16, 28, 4\n"
    "   18: SET_ALIAS  a4, r17\n"
    "   19: LOAD_IMM   r18, 4\n"
    "   20: SET_ALIAS  a1, r18\n"
    "   21: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
