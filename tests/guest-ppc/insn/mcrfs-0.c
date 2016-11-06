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
    "    6: BFEXT      r6, r5, 31, 1\n"
    "    7: BFEXT      r7, r5, 30, 1\n"
    "    8: BFEXT      r8, r5, 29, 1\n"
    "    9: BFEXT      r9, r5, 28, 1\n"
    "   10: SET_ALIAS  a2, r6\n"
    "   11: SET_ALIAS  a3, r7\n"
    "   12: SET_ALIAS  a4, r8\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: ANDI       r10, r5, 805306367\n"
    "   15: SRLI       r11, r10, 25\n"
    "   16: SRLI       r12, r10, 3\n"
    "   17: AND        r13, r11, r12\n"
    "   18: ANDI       r14, r13, 31\n"
    "   19: SGTUI      r15, r14, 0\n"
    "   20: SLLI       r16, r15, 30\n"
    "   21: OR         r17, r10, r16\n"
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
    "   41: ANDI       r34, r32, -522241\n"
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
