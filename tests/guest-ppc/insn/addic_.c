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
    0x34,0x60,0x12,0x34,  // addic. r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ADDI       r4, r3, 4660\n"
    "    4: GET_ALIAS  r5, a9\n"
    "    5: SLTUI      r6, r4, 4660\n"
    "    6: BFINS      r7, r5, r6, 29, 1\n"
    "    7: SLTSI      r8, r4, 0\n"
    "    8: SGTSI      r9, r4, 0\n"
    "    9: SEQI       r10, r4, 0\n"
    "   10: BFEXT      r11, r7, 31, 1\n"
    "   11: SET_ALIAS  a3, r4\n"
    "   12: SET_ALIAS  a4, r8\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: SET_ALIAS  a6, r10\n"
    "   15: SET_ALIAS  a7, r11\n"
    "   16: SET_ALIAS  a9, r7\n"
    "   17: LOAD_IMM   r12, 4\n"
    "   18: SET_ALIAS  a1, r12\n"
    "   19: GET_ALIAS  r13, a8\n"
    "   20: ANDI       r14, r13, 268435455\n"
    "   21: GET_ALIAS  r15, a4\n"
    "   22: SLLI       r16, r15, 31\n"
    "   23: OR         r17, r14, r16\n"
    "   24: GET_ALIAS  r18, a5\n"
    "   25: SLLI       r19, r18, 30\n"
    "   26: OR         r20, r17, r19\n"
    "   27: GET_ALIAS  r21, a6\n"
    "   28: SLLI       r22, r21, 29\n"
    "   29: OR         r23, r20, r22\n"
    "   30: GET_ALIAS  r24, a7\n"
    "   31: SLLI       r25, r24, 28\n"
    "   32: OR         r26, r23, r25\n"
    "   33: SET_ALIAS  a8, r26\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
