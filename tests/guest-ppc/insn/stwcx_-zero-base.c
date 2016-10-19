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
    0x7C,0x60,0x21,0x2D,  // stwcx. r3,0,r4
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_U8    r3, 948(r1)\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: SET_ALIAS  a6, r4\n"
    "    5: GET_ALIAS  r5, a9\n"
    "    6: BFEXT      r6, r5, 31, 1\n"
    "    7: GOTO_IF_Z  r3, L1\n"
    "    8: STORE_I8   948(r1), r4\n"
    "    9: LOAD       r7, 952(r1)\n"
    "   10: BSWAP      r8, r7\n"
    "   11: GET_ALIAS  r9, a2\n"
    "   12: BSWAP      r10, r9\n"
    "   13: GET_ALIAS  r11, a3\n"
    "   14: ZCAST      r12, r11\n"
    "   15: ADD        r13, r2, r12\n"
    "   16: CMPXCHG    r14, (r13), r8, r10\n"
    "   17: SEQ        r15, r14, r8\n"
    "   18: GOTO_IF_Z  r15, L1\n"
    "   19: LOAD_IMM   r16, 1\n"
    "   20: SET_ALIAS  a6, r16\n"
    "   21: LABEL      L1\n"
    "   22: SET_ALIAS  a4, r4\n"
    "   23: SET_ALIAS  a5, r4\n"
    "   24: SET_ALIAS  a7, r6\n"
    "   25: LOAD_IMM   r17, 4\n"
    "   26: SET_ALIAS  a1, r17\n"
    "   27: GET_ALIAS  r18, a8\n"
    "   28: ANDI       r19, r18, 268435455\n"
    "   29: GET_ALIAS  r20, a4\n"
    "   30: SLLI       r21, r20, 31\n"
    "   31: OR         r22, r19, r21\n"
    "   32: GET_ALIAS  r23, a5\n"
    "   33: SLLI       r24, r23, 30\n"
    "   34: OR         r25, r22, r24\n"
    "   35: GET_ALIAS  r26, a6\n"
    "   36: SLLI       r27, r26, 29\n"
    "   37: OR         r28, r25, r27\n"
    "   38: GET_ALIAS  r29, a7\n"
    "   39: SLLI       r30, r29, 28\n"
    "   40: OR         r31, r28, r30\n"
    "   41: SET_ALIAS  a8, r31\n"
    "   42: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,7] --> 1,3\n"
    "Block 1: 0 --> [8,18] --> 2,3\n"
    "Block 2: 1 --> [19,20] --> 3\n"
    "Block 3: 2,0,1 --> [21,42] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
