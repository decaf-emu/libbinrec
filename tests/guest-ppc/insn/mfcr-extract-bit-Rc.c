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
    0x7C,0x80,0x00,0x26,  // mfcr r4
    0x54,0x83,0x17,0xFF,  // rlwinm. r3,r4,2,31,31
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: SET_ALIAS  a3, r3\n"
    "    4: BFEXT      r4, r3, 30, 1\n"
    "    5: SET_ALIAS  a2, r4\n"
    "    6: SLTSI      r5, r4, 0\n"
    "    7: SGTSI      r6, r4, 0\n"
    "    8: SEQI       r7, r4, 0\n"
    "    9: GET_ALIAS  r8, a5\n"
    "   10: BFEXT      r9, r8, 31, 1\n"
    "   11: SLLI       r10, r5, 3\n"
    "   12: SLLI       r11, r6, 2\n"
    "   13: SLLI       r12, r7, 1\n"
    "   14: OR         r13, r10, r11\n"
    "   15: OR         r14, r12, r9\n"
    "   16: OR         r15, r13, r14\n"
    "   17: BFINS      r16, r3, r15, 28, 4\n"
    "   18: SET_ALIAS  a4, r16\n"
    "   19: LOAD_IMM   r17, 8\n"
    "   20: SET_ALIAS  a1, r17\n"
    "   21: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
