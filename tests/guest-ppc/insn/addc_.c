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
    "   14: GET_ALIAS  r14, a10\n"
    "   15: BFINS      r15, r14, r13, 29, 1\n"
    "   16: SLTSI      r16, r5, 0\n"
    "   17: SGTSI      r17, r5, 0\n"
    "   18: SEQI       r18, r5, 0\n"
    "   19: BFEXT      r19, r15, 31, 1\n"
    "   20: SET_ALIAS  a2, r5\n"
    "   21: SET_ALIAS  a5, r16\n"
    "   22: SET_ALIAS  a6, r17\n"
    "   23: SET_ALIAS  a7, r18\n"
    "   24: SET_ALIAS  a8, r19\n"
    "   25: SET_ALIAS  a10, r15\n"
    "   26: LOAD_IMM   r20, 4\n"
    "   27: SET_ALIAS  a1, r20\n"
    "   28: LABEL      L2\n"
    "   29: GET_ALIAS  r21, a9\n"
    "   30: ANDI       r22, r21, 268435455\n"
    "   31: GET_ALIAS  r23, a5\n"
    "   32: SLLI       r24, r23, 31\n"
    "   33: OR         r25, r22, r24\n"
    "   34: GET_ALIAS  r26, a6\n"
    "   35: SLLI       r27, r26, 30\n"
    "   36: OR         r28, r25, r27\n"
    "   37: GET_ALIAS  r29, a7\n"
    "   38: SLLI       r30, r29, 29\n"
    "   39: OR         r31, r28, r30\n"
    "   40: GET_ALIAS  r32, a8\n"
    "   41: SLLI       r33, r32, 28\n"
    "   42: OR         r34, r31, r33\n"
    "   43: SET_ALIAS  a9, r34\n"
    "   44: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 928(r1)\n"
    "Alias 10: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,27] --> 2\n"
    "Block 2: 1 --> [28,44] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
