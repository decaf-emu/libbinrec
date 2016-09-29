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
    0x7C,0x64,0x28,0x15,  // addc. r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: GET_ALIAS  r4, a4\n"
    "    5: ADD        r5, r3, r4\n"
    "    6: SRLI       r6, r3, 31\n"
    "    7: SRLI       r7, r4, 31\n"
    "    8: XOR        r8, r6, r7\n"
    "    9: SRLI       r9, r5, 31\n"
    "   10: XORI       r10, r9, 1\n"
    "   11: AND        r11, r6, r7\n"
    "   12: AND        r12, r8, r10\n"
    "   13: OR         r13, r11, r12\n"
    "   14: GET_ALIAS  r14, a6\n"
    "   15: BFINS      r15, r14, r13, 29, 1\n"
    "   16: SLTSI      r16, r5, 0\n"
    "   17: SGTSI      r17, r5, 0\n"
    "   18: SEQI       r18, r5, 0\n"
    "   19: BFEXT      r19, r15, 31, 1\n"
    "   20: SLLI       r20, r18, 1\n"
    "   21: OR         r21, r19, r20\n"
    "   22: SLLI       r22, r17, 2\n"
    "   23: OR         r23, r21, r22\n"
    "   24: SLLI       r24, r16, 3\n"
    "   25: OR         r25, r23, r24\n"
    "   26: SET_ALIAS  a2, r5\n"
    "   27: SET_ALIAS  a5, r25\n"
    "   28: SET_ALIAS  a6, r15\n"
    "   29: LOAD_IMM   r26, 4\n"
    "   30: SET_ALIAS  a1, r26\n"
    "   31: LABEL      L2\n"
    "   32: LOAD       r27, 928(r1)\n"
    "   33: ANDI       r28, r27, 268435455\n"
    "   34: GET_ALIAS  r29, a5\n"
    "   35: SLLI       r30, r29, 28\n"
    "   36: OR         r31, r28, r30\n"
    "   37: STORE      928(r1), r31\n"
    "   38: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,30] --> 2\n"
    "Block 2: 1 --> [31,38] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
