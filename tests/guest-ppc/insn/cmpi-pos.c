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
    0x2F,0x80,0x12,0x34,  // cmpi cr7,r0,4660
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: SLTSI      r4, r3, 4660\n"
    "    4: SGTSI      r5, r3, 4660\n"
    "    5: SEQI       r6, r3, 4660\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: BFEXT      r8, r7, 31, 1\n"
    "    8: GET_ALIAS  r9, a3\n"
    "    9: SLLI       r10, r4, 3\n"
    "   10: SLLI       r11, r5, 2\n"
    "   11: SLLI       r12, r6, 1\n"
    "   12: OR         r13, r10, r11\n"
    "   13: OR         r14, r12, r8\n"
    "   14: OR         r15, r13, r14\n"
    "   15: BFINS      r16, r9, r15, 0, 4\n"
    "   16: SET_ALIAS  a3, r16\n"
    "   17: LOAD_IMM   r17, 4\n"
    "   18: SET_ALIAS  a1, r17\n"
    "   19: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
