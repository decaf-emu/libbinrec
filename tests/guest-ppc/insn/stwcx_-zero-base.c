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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 30, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 29, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 28, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: LOAD_U8    r8, 948(r1)\n"
    "   12: LOAD_IMM   r9, 0\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: SET_ALIAS  a6, r9\n"
    "   15: SET_ALIAS  a7, r9\n"
    "   16: GET_ALIAS  r10, a9\n"
    "   17: BFEXT      r11, r10, 31, 1\n"
    "   18: SET_ALIAS  a8, r11\n"
    "   19: GOTO_IF_Z  r8, L1\n"
    "   20: STORE_I8   948(r1), r9\n"
    "   21: LOAD       r12, 952(r1)\n"
    "   22: BSWAP      r13, r12\n"
    "   23: GET_ALIAS  r14, a2\n"
    "   24: BSWAP      r15, r14\n"
    "   25: GET_ALIAS  r16, a3\n"
    "   26: ZCAST      r17, r16\n"
    "   27: ADD        r18, r2, r17\n"
    "   28: CMPXCHG    r19, (r18), r13, r15\n"
    "   29: SEQ        r20, r19, r13\n"
    "   30: GOTO_IF_Z  r20, L1\n"
    "   31: LOAD_IMM   r21, 1\n"
    "   32: SET_ALIAS  a7, r21\n"
    "   33: LABEL      L1\n"
    "   34: LOAD_IMM   r22, 4\n"
    "   35: SET_ALIAS  a1, r22\n"
    "   36: GET_ALIAS  r23, a4\n"
    "   37: ANDI       r24, r23, 268435455\n"
    "   38: GET_ALIAS  r25, a5\n"
    "   39: SLLI       r26, r25, 31\n"
    "   40: OR         r27, r24, r26\n"
    "   41: GET_ALIAS  r28, a6\n"
    "   42: SLLI       r29, r28, 30\n"
    "   43: OR         r30, r27, r29\n"
    "   44: GET_ALIAS  r31, a7\n"
    "   45: SLLI       r32, r31, 29\n"
    "   46: OR         r33, r30, r32\n"
    "   47: GET_ALIAS  r34, a8\n"
    "   48: SLLI       r35, r34, 28\n"
    "   49: OR         r36, r33, r35\n"
    "   50: SET_ALIAS  a4, r36\n"
    "   51: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,19] --> 1,3\n"
    "Block 1: 0 --> [20,30] --> 2,3\n"
    "Block 2: 1 --> [31,32] --> 3\n"
    "Block 3: 2,0,1 --> [33,51] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
