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
    "    1: LABEL      L1\n"
    "    2: GET_ALIAS  r2, a3\n"
    "    3: GET_ALIAS  r3, a4\n"
    "    4: ADD        r4, r2, r3\n"
    "    5: SRLI       r5, r2, 31\n"
    "    6: SRLI       r6, r3, 31\n"
    "    7: XOR        r7, r5, r6\n"
    "    8: SRLI       r8, r4, 31\n"
    "    9: XORI       r9, r8, 1\n"
    "   10: AND        r10, r5, r6\n"
    "   11: AND        r11, r7, r9\n"
    "   12: OR         r12, r10, r11\n"
    "   13: GET_ALIAS  r13, a6\n"
    "   14: BFINS      r14, r13, r12, 29, 1\n"
    "   15: SLTSI      r15, r4, 0\n"
    "   16: SGTSI      r16, r4, 0\n"
    "   17: SEQI       r17, r4, 0\n"
    "   18: BFEXT      r18, r14, 31, 1\n"
    "   19: SLLI       r19, r17, 1\n"
    "   20: OR         r20, r18, r19\n"
    "   21: SLLI       r21, r16, 2\n"
    "   22: OR         r22, r20, r21\n"
    "   23: SLLI       r23, r15, 3\n"
    "   24: OR         r24, r22, r23\n"
    "   25: SET_ALIAS  a2, r4\n"
    "   26: SET_ALIAS  a5, r24\n"
    "   27: SET_ALIAS  a6, r14\n"
    "   28: LOAD_IMM   r25, 4\n"
    "   29: SET_ALIAS  a1, r25\n"
    "   30: LABEL      L2\n"
    "   31: LOAD       r26, 928(r1)\n"
    "   32: ANDI       r27, r26, 268435455\n"
    "   33: GET_ALIAS  r28, a5\n"
    "   34: SLLI       r29, r28, 28\n"
    "   35: OR         r30, r27, r29\n"
    "   36: STORE      928(r1), r30\n"
    "   37: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 1\n"
    "Block 1: 0 --> [1,29] --> 2\n"
    "Block 2: 1 --> [30,37] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
