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
    0x7C,0x64,0x29,0x2D,  // stwcx. r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: LOAD_U8    r3, 948(r1)\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a7, r4\n"
    "    6: GET_ALIAS  r5, a10\n"
    "    7: BFEXT      r6, r5, 31, 1\n"
    "    8: GOTO_IF_Z  r3, L3\n"
    "    9: STORE_I8   948(r1), r4\n"
    "   10: LOAD       r7, 952(r1)\n"
    "   11: BSWAP      r8, r7\n"
    "   12: GET_ALIAS  r9, a2\n"
    "   13: BSWAP      r10, r9\n"
    "   14: GET_ALIAS  r11, a3\n"
    "   15: GET_ALIAS  r12, a4\n"
    "   16: ADD        r13, r11, r12\n"
    "   17: ZCAST      r14, r13\n"
    "   18: ADD        r15, r2, r14\n"
    "   19: CMPXCHG    r16, (r15), r8, r10\n"
    "   20: SEQ        r17, r16, r8\n"
    "   21: GOTO_IF_Z  r17, L3\n"
    "   22: LOAD_IMM   r18, 1\n"
    "   23: SET_ALIAS  a7, r18\n"
    "   24: LABEL      L3\n"
    "   25: SET_ALIAS  a5, r4\n"
    "   26: SET_ALIAS  a6, r4\n"
    "   27: SET_ALIAS  a8, r6\n"
    "   28: LOAD_IMM   r19, 4\n"
    "   29: SET_ALIAS  a1, r19\n"
    "   30: LABEL      L2\n"
    "   31: GET_ALIAS  r20, a9\n"
    "   32: ANDI       r21, r20, 268435455\n"
    "   33: GET_ALIAS  r22, a5\n"
    "   34: SLLI       r23, r22, 31\n"
    "   35: OR         r24, r21, r23\n"
    "   36: GET_ALIAS  r25, a6\n"
    "   37: SLLI       r26, r25, 30\n"
    "   38: OR         r27, r24, r26\n"
    "   39: GET_ALIAS  r28, a7\n"
    "   40: SLLI       r29, r28, 29\n"
    "   41: OR         r30, r27, r29\n"
    "   42: GET_ALIAS  r31, a8\n"
    "   43: SLLI       r32, r31, 28\n"
    "   44: OR         r33, r30, r32\n"
    "   45: SET_ALIAS  a9, r33\n"
    "   46: RETURN\n"
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
    "Block 1: 0 --> [2,8] --> 2,4\n"
    "Block 2: 1 --> [9,21] --> 3,4\n"
    "Block 3: 2 --> [22,23] --> 4\n"
    "Block 4: 3,1,2 --> [24,29] --> 5\n"
    "Block 5: 4 --> [30,46] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
