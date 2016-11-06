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
    0xFF,0x98,0x00,0x80,  // mcrfs cr7,cr6
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a8, r4\n"
    "    5: GET_ALIAS  r5, a7\n"
    "    6: BFEXT      r6, r5, 7, 1\n"
    "    7: BFEXT      r7, r5, 6, 1\n"
    "    8: BFEXT      r8, r5, 5, 1\n"
    "    9: BFEXT      r9, r5, 4, 1\n"
    "   10: SET_ALIAS  a2, r6\n"
    "   11: SET_ALIAS  a3, r7\n"
    "   12: SET_ALIAS  a4, r8\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: LOAD_IMM   r10, 4\n"
    "   15: SET_ALIAS  a1, r10\n"
    "   16: GET_ALIAS  r11, a6\n"
    "   17: ANDI       r12, r11, -16\n"
    "   18: GET_ALIAS  r13, a2\n"
    "   19: SLLI       r14, r13, 3\n"
    "   20: OR         r15, r12, r14\n"
    "   21: GET_ALIAS  r16, a3\n"
    "   22: SLLI       r17, r16, 2\n"
    "   23: OR         r18, r15, r17\n"
    "   24: GET_ALIAS  r19, a4\n"
    "   25: SLLI       r20, r19, 1\n"
    "   26: OR         r21, r18, r20\n"
    "   27: GET_ALIAS  r22, a5\n"
    "   28: OR         r23, r21, r22\n"
    "   29: SET_ALIAS  a6, r23\n"
    "   30: GET_ALIAS  r24, a7\n"
    "   31: GET_ALIAS  r25, a8\n"
    "   32: ANDI       r26, r24, -1611134977\n"
    "   33: SLLI       r27, r25, 12\n"
    "   34: OR         r28, r26, r27\n"
    "   35: SET_ALIAS  a7, r28\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "Alias 8: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
