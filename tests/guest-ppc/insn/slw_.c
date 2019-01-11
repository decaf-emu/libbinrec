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
    0x7C,0x83,0x28,0x31,  // slw. r3,r4,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: ANDI       r5, r4, 63\n"
    "    5: ZCAST      r6, r3\n"
    "    6: SLL        r7, r6, r5\n"
    "    7: ZCAST      r8, r7\n"
    "    8: SET_ALIAS  a2, r8\n"
    "    9: SLTSI      r9, r8, 0\n"
    "   10: SGTSI      r10, r8, 0\n"
    "   11: SEQI       r11, r8, 0\n"
    "   12: GET_ALIAS  r12, a6\n"
    "   13: BFEXT      r13, r12, 31, 1\n"
    "   14: GET_ALIAS  r14, a5\n"
    "   15: SLLI       r15, r9, 3\n"
    "   16: SLLI       r16, r10, 2\n"
    "   17: SLLI       r17, r11, 1\n"
    "   18: OR         r18, r15, r16\n"
    "   19: OR         r19, r17, r13\n"
    "   20: OR         r20, r18, r19\n"
    "   21: BFINS      r21, r14, r20, 28, 4\n"
    "   22: SET_ALIAS  a5, r21\n"
    "   23: LOAD_IMM   r22, 4\n"
    "   24: SET_ALIAS  a1, r22\n"
    "   25: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
