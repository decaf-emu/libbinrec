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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a5\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: SET_ALIAS  a6, r4\n"
    "    5: BFEXT      r5, r3, 30, 1\n"
    "    6: SET_ALIAS  a7, r5\n"
    "    7: BFEXT      r6, r3, 29, 1\n"
    "    8: SET_ALIAS  a8, r6\n"
    "    9: BFEXT      r7, r3, 28, 1\n"
    "   10: SET_ALIAS  a9, r7\n"
    "   11: GET_ALIAS  r8, a10\n"
    "   12: BFEXT      r9, r8, 31, 1\n"
    "   13: SET_ALIAS  a11, r9\n"
    "   14: LOAD_U8    r10, 948(r1)\n"
    "   15: LOAD_IMM   r11, 0\n"
    "   16: GET_ALIAS  r12, a11\n"
    "   17: SET_ALIAS  a6, r11\n"
    "   18: SET_ALIAS  a7, r11\n"
    "   19: SET_ALIAS  a8, r11\n"
    "   20: SET_ALIAS  a9, r12\n"
    "   21: GOTO_IF_Z  r10, L1\n"
    "   22: STORE_I8   948(r1), r11\n"
    "   23: LOAD       r13, 952(r1)\n"
    "   24: GET_ALIAS  r14, a2\n"
    "   25: BSWAP      r15, r14\n"
    "   26: GET_ALIAS  r16, a3\n"
    "   27: GET_ALIAS  r17, a4\n"
    "   28: ADD        r18, r16, r17\n"
    "   29: ZCAST      r19, r18\n"
    "   30: ADD        r20, r2, r19\n"
    "   31: CMPXCHG    r21, (r20), r13, r15\n"
    "   32: SEQ        r22, r21, r13\n"
    "   33: GOTO_IF_Z  r22, L1\n"
    "   34: LOAD_IMM   r23, 1\n"
    "   35: SET_ALIAS  a8, r23\n"
    "   36: LABEL      L1\n"
    "   37: LOAD_IMM   r24, 4\n"
    "   38: SET_ALIAS  a1, r24\n"
    "   39: GET_ALIAS  r25, a5\n"
    "   40: ANDI       r26, r25, 268435455\n"
    "   41: GET_ALIAS  r27, a6\n"
    "   42: SLLI       r28, r27, 31\n"
    "   43: OR         r29, r26, r28\n"
    "   44: GET_ALIAS  r30, a7\n"
    "   45: SLLI       r31, r30, 30\n"
    "   46: OR         r32, r29, r31\n"
    "   47: GET_ALIAS  r33, a8\n"
    "   48: SLLI       r34, r33, 29\n"
    "   49: OR         r35, r32, r34\n"
    "   50: GET_ALIAS  r36, a9\n"
    "   51: SLLI       r37, r36, 28\n"
    "   52: OR         r38, r35, r37\n"
    "   53: SET_ALIAS  a5, r38\n"
    "   54: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32 @ 940(r1)\n"
    "Alias 11: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,21] --> 1,3\n"
    "Block 1: 0 --> [22,33] --> 2,3\n"
    "Block 2: 1 --> [34,35] --> 3\n"
    "Block 3: 2,0,1 --> [36,54] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
