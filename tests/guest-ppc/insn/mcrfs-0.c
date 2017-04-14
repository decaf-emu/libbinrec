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
    "    2: GET_ALIAS  r3, a3\n"
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
    "   14: GET_ALIAS  r15, a2\n"
    "   15: SLLI       r16, r13, 3\n"
    "   16: SLLI       r17, r12, 2\n"
    "   17: SLLI       r18, r5, 1\n"
    "   18: OR         r19, r16, r17\n"
    "   19: OR         r20, r18, r14\n"
    "   20: OR         r21, r19, r20\n"
    "   21: BFINS      r22, r15, r21, 0, 4\n"
    "   22: SET_ALIAS  a2, r22\n"
    "   23: ANDI       r23, r3, 1879048191\n"
    "   24: SET_ALIAS  a3, r23\n"
    "   25: LOAD_IMM   r24, 4\n"
    "   26: SET_ALIAS  a1, r24\n"
    "   27: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,27] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
