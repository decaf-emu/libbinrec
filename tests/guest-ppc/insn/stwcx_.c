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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_U8    r3, 948(r1)\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: GET_ALIAS  r5, a6\n"
    "    5: BFEXT      r6, r5, 31, 1\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: BFINS      r8, r7, r6, 28, 4\n"
    "    8: SET_ALIAS  a5, r8\n"
    "    9: GOTO_IF_Z  r3, L1\n"
    "   10: STORE_I8   948(r1), r4\n"
    "   11: LOAD       r9, 952(r1)\n"
    "   12: BSWAP      r10, r9\n"
    "   13: GET_ALIAS  r11, a2\n"
    "   14: BSWAP      r12, r11\n"
    "   15: GET_ALIAS  r13, a3\n"
    "   16: GET_ALIAS  r14, a4\n"
    "   17: ADD        r15, r13, r14\n"
    "   18: ZCAST      r16, r15\n"
    "   19: ADD        r17, r2, r16\n"
    "   20: CMPXCHG    r18, (r17), r10, r12\n"
    "   21: SEQ        r19, r18, r10\n"
    "   22: GOTO_IF_Z  r19, L1\n"
    "   23: GET_ALIAS  r20, a5\n"
    "   24: ORI        r21, r20, 536870912\n"
    "   25: SET_ALIAS  a5, r21\n"
    "   26: LABEL      L1\n"
    "   27: LOAD_IMM   r22, 4\n"
    "   28: SET_ALIAS  a1, r22\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,3\n"
    "Block 1: 0 --> [10,22] --> 2,3\n"
    "Block 2: 1 --> [23,25] --> 3\n"
    "Block 3: 2,0,1 --> [26,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
