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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r5, a3\n"
    "    7: GET_ALIAS  r6, a4\n"
    "    8: ADD        r7, r5, r6\n"
    "    9: SRLI       r8, r5, 31\n"
    "   10: SRLI       r9, r6, 31\n"
    "   11: XOR        r10, r8, r9\n"
    "   12: SRLI       r11, r7, 31\n"
    "   13: XORI       r12, r11, 1\n"
    "   14: AND        r13, r8, r9\n"
    "   15: AND        r14, r10, r12\n"
    "   16: OR         r15, r13, r14\n"
    "   17: GET_ALIAS  r16, a6\n"
    "   18: BFINS      r17, r16, r15, 29, 1\n"
    "   19: SLTSI      r18, r7, 0\n"
    "   20: SGTSI      r19, r7, 0\n"
    "   21: SEQI       r20, r7, 0\n"
    "   22: BFEXT      r21, r17, 31, 1\n"
    "   23: SLLI       r22, r20, 1\n"
    "   24: OR         r23, r21, r22\n"
    "   25: SLLI       r24, r19, 2\n"
    "   26: OR         r25, r23, r24\n"
    "   27: SLLI       r26, r18, 3\n"
    "   28: OR         r27, r25, r26\n"
    "   29: SET_ALIAS  a2, r7\n"
    "   30: SET_ALIAS  a5, r27\n"
    "   31: SET_ALIAS  a6, r17\n"
    "   32: LOAD_IMM   r28, 4\n"
    "   33: SET_ALIAS  a1, r28\n"
    "   34: LABEL      L2\n"
    "   35: LOAD       r29, 928(r1)\n"
    "   36: ANDI       r30, r29, 268435455\n"
    "   37: GET_ALIAS  r31, a5\n"
    "   38: SLLI       r32, r31, 28\n"
    "   39: OR         r33, r30, r32\n"
    "   40: STORE      928(r1), r33\n"
    "   41: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,33] --> 2\n"
    "Block 2: 1 --> [34,41] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
