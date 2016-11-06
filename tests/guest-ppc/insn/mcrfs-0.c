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
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a8, r4\n"
    "    5: GET_ALIAS  r5, a7\n"
    "    6: ANDI       r6, r5, 33031936\n"
    "    7: SGTUI      r7, r6, 0\n"
    "    8: BFEXT      r8, r5, 25, 4\n"
    "    9: SLLI       r9, r7, 4\n"
    "   10: SRLI       r10, r5, 3\n"
    "   11: OR         r11, r8, r9\n"
    "   12: AND        r12, r11, r10\n"
    "   13: ANDI       r13, r12, 31\n"
    "   14: SGTUI      r14, r13, 0\n"
    "   15: BFEXT      r15, r5, 31, 1\n"
    "   16: BFEXT      r16, r5, 28, 1\n"
    "   17: SET_ALIAS  a2, r15\n"
    "   18: SET_ALIAS  a3, r14\n"
    "   19: SET_ALIAS  a4, r7\n"
    "   20: SET_ALIAS  a5, r16\n"
    "   21: ANDI       r17, r5, 1879048191\n"
    "   22: SET_ALIAS  a7, r17\n"
    "   23: LOAD_IMM   r18, 4\n"
    "   24: SET_ALIAS  a1, r18\n"
    "   25: GET_ALIAS  r19, a6\n"
    "   26: ANDI       r20, r19, -16\n"
    "   27: GET_ALIAS  r21, a2\n"
    "   28: SLLI       r22, r21, 3\n"
    "   29: OR         r23, r20, r22\n"
    "   30: GET_ALIAS  r24, a3\n"
    "   31: SLLI       r25, r24, 2\n"
    "   32: OR         r26, r23, r25\n"
    "   33: GET_ALIAS  r27, a4\n"
    "   34: SLLI       r28, r27, 1\n"
    "   35: OR         r29, r26, r28\n"
    "   36: GET_ALIAS  r30, a5\n"
    "   37: OR         r31, r29, r30\n"
    "   38: SET_ALIAS  a6, r31\n"
    "   39: GET_ALIAS  r32, a7\n"
    "   40: GET_ALIAS  r33, a8\n"
    "   41: ANDI       r34, r32, -1611134977\n"
    "   42: SLLI       r35, r33, 12\n"
    "   43: OR         r36, r34, r35\n"
    "   44: SET_ALIAS  a7, r36\n"
    "   45: RETURN\n"
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
    "Block 0: <none> --> [0,45] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
