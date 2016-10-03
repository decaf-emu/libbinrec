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
    "    2: LOAD       r3, 928(r1)\n"
    "    3: BFEXT      r4, r3, 28, 4\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: LABEL      L1\n"
    "    6: LOAD_U8    r5, 948(r1)\n"
    "    7: GET_ALIAS  r6, a5\n"
    "    8: BFEXT      r7, r6, 31, 1\n"
    "    9: SET_ALIAS  a4, r7\n"
    "   10: GOTO_IF_Z  r5, L3\n"
    "   11: LOAD_IMM   r8, 0\n"
    "   12: STORE_I8   948(r1), r8\n"
    "   13: LOAD       r9, 952(r1)\n"
    "   14: BSWAP      r10, r9\n"
    "   15: GET_ALIAS  r11, a2\n"
    "   16: BSWAP      r12, r11\n"
    "   17: GET_ALIAS  r13, a3\n"
    "   18: ZCAST      r14, r13\n"
    "   19: ADD        r15, r2, r14\n"
    "   20: CMPXCHG    r16, (r15), r10, r12\n"
    "   21: SEQ        r17, r16, r10\n"
    "   22: GOTO_IF_Z  r17, L3\n"
    "   23: ORI        r18, r7, 2\n"
    "   24: SET_ALIAS  a4, r18\n"
    "   25: LABEL      L3\n"
    "   26: LOAD_IMM   r19, 4\n"
    "   27: SET_ALIAS  a1, r19\n"
    "   28: LABEL      L2\n"
    "   29: LOAD       r20, 928(r1)\n"
    "   30: ANDI       r21, r20, 268435455\n"
    "   31: GET_ALIAS  r22, a4\n"
    "   32: SLLI       r23, r22, 28\n"
    "   33: OR         r24, r21, r23\n"
    "   34: STORE      928(r1), r24\n"
    "   35: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> 1\n"
    "Block 1: 0 --> [5,10] --> 2,4\n"
    "Block 2: 1 --> [11,22] --> 3,4\n"
    "Block 3: 2 --> [23,24] --> 4\n"
    "Block 4: 3,1,2 --> [25,27] --> 5\n"
    "Block 5: 4 --> [28,35] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
