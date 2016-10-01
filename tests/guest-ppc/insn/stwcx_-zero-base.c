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
    "    2: LABEL      L1\n"
    "    3: LOAD_U8    r3, 948(r1)\n"
    "    4: GET_ALIAS  r4, a5\n"
    "    5: BFEXT      r5, r4, 31, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: GOTO_IF_Z  r3, L3\n"
    "    8: LOAD_IMM   r6, 0\n"
    "    9: STORE_I8   948(r1), r6\n"
    "   10: LOAD       r7, 952(r1)\n"
    "   11: BSWAP      r8, r7\n"
    "   12: GET_ALIAS  r9, a2\n"
    "   13: BSWAP      r10, r9\n"
    "   14: GET_ALIAS  r11, a3\n"
    "   15: ZCAST      r12, r11\n"
    "   16: ADD        r13, r2, r12\n"
    "   17: CMPXCHG    r14, (r13), r8, r10\n"
    "   18: SEQ        r15, r14, r8\n"
    "   19: GOTO_IF_Z  r15, L3\n"
    "   20: ORI        r16, r5, 2\n"
    "   21: SET_ALIAS  a4, r16\n"
    "   22: LABEL      L3\n"
    "   23: LOAD_IMM   r17, 4\n"
    "   24: SET_ALIAS  a1, r17\n"
    "   25: LABEL      L2\n"
    "   26: LOAD       r18, 928(r1)\n"
    "   27: ANDI       r19, r18, 268435455\n"
    "   28: GET_ALIAS  r20, a4\n"
    "   29: SLLI       r21, r20, 28\n"
    "   30: OR         r22, r19, r21\n"
    "   31: STORE      928(r1), r22\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,7] --> 2,4\n"
    "Block 2: 1 --> [8,19] --> 3,4\n"
    "Block 3: 2 --> [20,21] --> 4\n"
    "Block 4: 3,1,2 --> [22,24] --> 5\n"
    "Block 5: 4 --> [25,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
