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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a2\n"
    "    7: ADDI       r6, r5, 4660\n"
    "    8: GET_ALIAS  r7, a5\n"
    "    9: SLTUI      r8, r6, 4660\n"
    "   10: BFINS      r9, r7, r8, 29, 1\n"
    "   11: SLTSI      r10, r6, 0\n"
    "   12: SGTSI      r11, r6, 0\n"
    "   13: SEQI       r12, r6, 0\n"
    "   14: BFEXT      r13, r9, 31, 1\n"
    "   15: SLLI       r14, r12, 1\n"
    "   16: OR         r15, r13, r14\n"
    "   17: SLLI       r16, r11, 2\n"
    "   18: OR         r17, r15, r16\n"
    "   19: SLLI       r18, r10, 3\n"
    "   20: OR         r19, r17, r18\n"
    "   21: SET_ALIAS  a3, r6\n"
    "   22: SET_ALIAS  a4, r19\n"
    "   23: SET_ALIAS  a5, r9\n"
    "   24: LOAD_IMM   r20, 4\n"
    "   25: SET_ALIAS  a1, r20\n"
    "   26: LABEL      L2\n"
    "   27: LOAD       r21, 928(r1)\n"
    "   28: ANDI       r22, r21, 268435455\n"
    "   29: GET_ALIAS  r23, a4\n"
    "   30: SLLI       r24, r23, 28\n"
    "   31: OR         r25, r22, r24\n"
    "   32: STORE      928(r1), r25\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,25] --> 2\n"
    "Block 2: 1 --> [26,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
