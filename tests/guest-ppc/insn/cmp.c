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
    0x7F,0x83,0x20,0x00,  // cmp cr7,r3,r4
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a2\n"
    "   12: GET_ALIAS  r12, a3\n"
    "   13: SLTS       r9, r8, r12\n"
    "   14: SGTS       r10, r8, r12\n"
    "   15: SEQ        r11, r8, r12\n"
    "   16: GET_ALIAS  r13, a9\n"
    "   17: BFEXT      r14, r13, 31, 1\n"
    "   18: SET_ALIAS  a5, r9\n"
    "   19: SET_ALIAS  a6, r10\n"
    "   20: SET_ALIAS  a7, r11\n"
    "   21: SET_ALIAS  a8, r14\n"
    "   22: LOAD_IMM   r15, 4\n"
    "   23: SET_ALIAS  a1, r15\n"
    "   24: GET_ALIAS  r16, a4\n"
    "   25: ANDI       r17, r16, -16\n"
    "   26: GET_ALIAS  r18, a5\n"
    "   27: SLLI       r19, r18, 3\n"
    "   28: OR         r20, r17, r19\n"
    "   29: GET_ALIAS  r21, a6\n"
    "   30: SLLI       r22, r21, 2\n"
    "   31: OR         r23, r20, r22\n"
    "   32: GET_ALIAS  r24, a7\n"
    "   33: SLLI       r25, r24, 1\n"
    "   34: OR         r26, r23, r25\n"
    "   35: GET_ALIAS  r27, a8\n"
    "   36: OR         r28, r26, r27\n"
    "   37: SET_ALIAS  a4, r28\n"
    "   38: RETURN\n"
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
    "Block 0: <none> --> [0,38] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
