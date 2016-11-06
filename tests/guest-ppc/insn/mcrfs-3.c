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
    0xFF,0x8C,0x00,0x80,  // mcrfs cr7,cr3
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
    "    6: GET_ALIAS  r6, a8\n"
    "    7: BFEXT      r7, r5, 19, 1\n"
    "    8: BFEXT      r8, r6, 6, 1\n"
    "    9: BFEXT      r9, r6, 5, 1\n"
    "   10: BFEXT      r10, r6, 4, 1\n"
    "   11: SET_ALIAS  a2, r7\n"
    "   12: SET_ALIAS  a3, r8\n"
    "   13: SET_ALIAS  a4, r9\n"
    "   14: SET_ALIAS  a5, r10\n"
    "   15: ANDI       r11, r5, -524289\n"
    "   16: SET_ALIAS  a7, r11\n"
    "   17: LOAD_IMM   r12, 4\n"
    "   18: SET_ALIAS  a1, r12\n"
    "   19: GET_ALIAS  r13, a6\n"
    "   20: ANDI       r14, r13, -16\n"
    "   21: GET_ALIAS  r15, a2\n"
    "   22: SLLI       r16, r15, 3\n"
    "   23: OR         r17, r14, r16\n"
    "   24: GET_ALIAS  r18, a3\n"
    "   25: SLLI       r19, r18, 2\n"
    "   26: OR         r20, r17, r19\n"
    "   27: GET_ALIAS  r21, a4\n"
    "   28: SLLI       r22, r21, 1\n"
    "   29: OR         r23, r20, r22\n"
    "   30: GET_ALIAS  r24, a5\n"
    "   31: OR         r25, r23, r24\n"
    "   32: SET_ALIAS  a6, r25\n"
    "   33: GET_ALIAS  r26, a7\n"
    "   34: GET_ALIAS  r27, a8\n"
    "   35: ANDI       r28, r26, -1611134977\n"
    "   36: SLLI       r29, r27, 12\n"
    "   37: OR         r30, r28, r29\n"
    "   38: SET_ALIAS  a7, r30\n"
    "   39: RETURN\n"
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
    "Block 0: <none> --> [0,39] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
