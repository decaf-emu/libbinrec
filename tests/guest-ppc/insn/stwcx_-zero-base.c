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
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_U8    r3, 948(r1)\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: BFEXT      r6, r5, 31, 1\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: BFINS      r8, r7, r6, 28, 4\n"
    "    8: SET_ALIAS  a4, r8\n"
    "    9: GOTO_IF_Z  r3, L1\n"
    "   10: STORE_I8   948(r1), r4\n"
    "   11: LOAD       r9, 952(r1)\n"
    "   12: GET_ALIAS  r10, a2\n"
    "   13: BSWAP      r11, r10\n"
    "   14: GET_ALIAS  r12, a3\n"
    "   15: ZCAST      r13, r12\n"
    "   16: ADD        r14, r2, r13\n"
    "   17: CMPXCHG    r15, (r14), r9, r11\n"
    "   18: SEQ        r16, r15, r9\n"
    "   19: GOTO_IF_Z  r16, L1\n"
    "   20: GET_ALIAS  r17, a4\n"
    "   21: ORI        r18, r17, 536870912\n"
    "   22: SET_ALIAS  a4, r18\n"
    "   23: LABEL      L1\n"
    "   24: LOAD_IMM   r19, 4\n"
    "   25: SET_ALIAS  a1, r19\n"
    "   26: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,3\n"
    "Block 1: 0 --> [10,19] --> 2,3\n"
    "Block 2: 1 --> [20,22] --> 3\n"
    "Block 3: 2,0,1 --> [23,26] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
