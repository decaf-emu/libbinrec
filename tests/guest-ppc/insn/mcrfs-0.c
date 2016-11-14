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
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a5, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a6, r7\n"
    "   11: GET_ALIAS  r8, a7\n"
    "   12: ANDI       r9, r8, 33031936\n"
    "   13: SGTUI      r10, r9, 0\n"
    "   14: BFEXT      r11, r8, 25, 4\n"
    "   15: SLLI       r12, r10, 4\n"
    "   16: SRLI       r13, r8, 3\n"
    "   17: OR         r14, r11, r12\n"
    "   18: AND        r15, r14, r13\n"
    "   19: ANDI       r16, r15, 31\n"
    "   20: SGTUI      r17, r16, 0\n"
    "   21: BFEXT      r18, r8, 31, 1\n"
    "   22: BFEXT      r19, r8, 28, 1\n"
    "   23: SET_ALIAS  a3, r18\n"
    "   24: SET_ALIAS  a4, r17\n"
    "   25: SET_ALIAS  a5, r10\n"
    "   26: SET_ALIAS  a6, r19\n"
    "   27: ANDI       r20, r8, 1879048191\n"
    "   28: SET_ALIAS  a7, r20\n"
    "   29: LOAD_IMM   r21, 4\n"
    "   30: SET_ALIAS  a1, r21\n"
    "   31: GET_ALIAS  r22, a2\n"
    "   32: ANDI       r23, r22, -16\n"
    "   33: GET_ALIAS  r24, a3\n"
    "   34: SLLI       r25, r24, 3\n"
    "   35: OR         r26, r23, r25\n"
    "   36: GET_ALIAS  r27, a4\n"
    "   37: SLLI       r28, r27, 2\n"
    "   38: OR         r29, r26, r28\n"
    "   39: GET_ALIAS  r30, a5\n"
    "   40: SLLI       r31, r30, 1\n"
    "   41: OR         r32, r29, r31\n"
    "   42: GET_ALIAS  r33, a6\n"
    "   43: OR         r34, r32, r33\n"
    "   44: SET_ALIAS  a2, r34\n"
    "   45: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,45] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
