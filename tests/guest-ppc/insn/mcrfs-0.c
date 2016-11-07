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
    0xFF,0x80,0x00,0x80,  // mcrfs cr7,cr0
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: ANDI       r4, r3, 33031936\n"
    "    4: SGTUI      r5, r4, 0\n"
    "    5: BFEXT      r6, r3, 25, 4\n"
    "    6: SLLI       r7, r5, 4\n"
    "    7: SRLI       r8, r3, 3\n"
    "    8: OR         r9, r6, r7\n"
    "    9: AND        r10, r9, r8\n"
    "   10: ANDI       r11, r10, 31\n"
    "   11: SGTUI      r12, r11, 0\n"
    "   12: BFEXT      r13, r3, 31, 1\n"
    "   13: BFEXT      r14, r3, 28, 1\n"
    "   14: SET_ALIAS  a2, r13\n"
    "   15: SET_ALIAS  a3, r12\n"
    "   16: SET_ALIAS  a4, r5\n"
    "   17: SET_ALIAS  a5, r14\n"
    "   18: ANDI       r15, r3, 1879048191\n"
    "   19: SET_ALIAS  a7, r15\n"
    "   20: LOAD_IMM   r16, 4\n"
    "   21: SET_ALIAS  a1, r16\n"
    "   22: GET_ALIAS  r17, a6\n"
    "   23: ANDI       r18, r17, -16\n"
    "   24: GET_ALIAS  r19, a2\n"
    "   25: SLLI       r20, r19, 3\n"
    "   26: OR         r21, r18, r20\n"
    "   27: GET_ALIAS  r22, a3\n"
    "   28: SLLI       r23, r22, 2\n"
    "   29: OR         r24, r21, r23\n"
    "   30: GET_ALIAS  r25, a4\n"
    "   31: SLLI       r26, r25, 1\n"
    "   32: OR         r27, r24, r26\n"
    "   33: GET_ALIAS  r28, a5\n"
    "   34: OR         r29, r27, r28\n"
    "   35: SET_ALIAS  a6, r29\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32, no bound storage\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
