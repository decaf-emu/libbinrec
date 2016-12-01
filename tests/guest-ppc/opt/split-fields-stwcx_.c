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
    "   11: LOAD_U8    r8, 948(r1)\n"
    "   12: LOAD_IMM   r9, 0\n"
    "   13: GET_ALIAS  r10, a10\n"
    "   14: BFEXT      r11, r10, 31, 1\n"
    "   15: SET_ALIAS  a6, r9\n"
    "   16: SET_ALIAS  a7, r9\n"
    "   17: SET_ALIAS  a8, r9\n"
    "   18: SET_ALIAS  a9, r11\n"
    "   19: GOTO_IF_Z  r8, L1\n"
    "   20: STORE_I8   948(r1), r9\n"
    "   21: LOAD       r12, 952(r1)\n"
    "   22: GET_ALIAS  r13, a2\n"
    "   23: BSWAP      r14, r13\n"
    "   24: GET_ALIAS  r15, a3\n"
    "   25: GET_ALIAS  r16, a4\n"
    "   26: ADD        r17, r15, r16\n"
    "   27: ZCAST      r18, r17\n"
    "   28: ADD        r19, r2, r18\n"
    "   29: CMPXCHG    r20, (r19), r12, r14\n"
    "   30: SEQ        r21, r20, r12\n"
    "   31: GOTO_IF_Z  r21, L1\n"
    "   32: LOAD_IMM   r22, 1\n"
    "   33: SET_ALIAS  a8, r22\n"
    "   34: LABEL      L1\n"
    "   35: LOAD_IMM   r23, 4\n"
    "   36: SET_ALIAS  a1, r23\n"
    "   37: GET_ALIAS  r24, a5\n"
    "   38: ANDI       r25, r24, 268435455\n"
    "   39: GET_ALIAS  r26, a6\n"
    "   40: SLLI       r27, r26, 31\n"
    "   41: OR         r28, r25, r27\n"
    "   42: GET_ALIAS  r29, a7\n"
    "   43: SLLI       r30, r29, 30\n"
    "   44: OR         r31, r28, r30\n"
    "   45: GET_ALIAS  r32, a8\n"
    "   46: SLLI       r33, r32, 29\n"
    "   47: OR         r34, r31, r33\n"
    "   48: GET_ALIAS  r35, a9\n"
    "   49: SLLI       r36, r35, 28\n"
    "   50: OR         r37, r34, r36\n"
    "   51: SET_ALIAS  a5, r37\n"
    "   52: RETURN\n"
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
    "\n"
    "Block 0: <none> --> [0,19] --> 1,3\n"
    "Block 1: 0 --> [20,31] --> 2,3\n"
    "Block 2: 1 --> [32,33] --> 3\n"
    "Block 3: 2,0,1 --> [34,52] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
