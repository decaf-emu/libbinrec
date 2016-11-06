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
    0xFF,0x88,0x00,0x80,  // mcrfs cr7,cr2
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
    "    6: BFEXT      r6, r5, 23, 1\n"
    "    7: BFEXT      r7, r5, 22, 1\n"
    "    8: BFEXT      r8, r5, 21, 1\n"
    "    9: BFEXT      r9, r5, 20, 1\n"
    "   10: SET_ALIAS  a2, r6\n"
    "   11: SET_ALIAS  a3, r7\n"
    "   12: SET_ALIAS  a4, r8\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: ANDI       r10, r5, -15728641\n"
    "   15: SET_ALIAS  a7, r10\n"
    "   16: LOAD_IMM   r11, 4\n"
    "   17: SET_ALIAS  a1, r11\n"
    "   18: GET_ALIAS  r12, a6\n"
    "   19: ANDI       r13, r12, -16\n"
    "   20: GET_ALIAS  r14, a2\n"
    "   21: SLLI       r15, r14, 3\n"
    "   22: OR         r16, r13, r15\n"
    "   23: GET_ALIAS  r17, a3\n"
    "   24: SLLI       r18, r17, 2\n"
    "   25: OR         r19, r16, r18\n"
    "   26: GET_ALIAS  r20, a4\n"
    "   27: SLLI       r21, r20, 1\n"
    "   28: OR         r22, r19, r21\n"
    "   29: GET_ALIAS  r23, a5\n"
    "   30: OR         r24, r22, r23\n"
    "   31: SET_ALIAS  a6, r24\n"
    "   32: GET_ALIAS  r25, a7\n"
    "   33: GET_ALIAS  r26, a8\n"
    "   34: ANDI       r27, r25, -1611134977\n"
    "   35: SLLI       r28, r26, 12\n"
    "   36: OR         r29, r27, r28\n"
    "   37: SET_ALIAS  a7, r29\n"
    "   38: RETURN\n"
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
    "Block 0: <none> --> [0,38] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
