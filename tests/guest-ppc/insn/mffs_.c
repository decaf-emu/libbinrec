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
    0xFC,0x20,0x04,0x8F,  // mffs. f1
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: ANDI       r4, r3, 33031936\n"
    "    4: SGTUI      r5, r4, 0\n"
    "    5: BFEXT      r6, r3, 25, 4\n"
    "    6: SLLI       r7, r5, 4\n"
    "    7: SRLI       r8, r3, 3\n"
    "    8: OR         r9, r6, r7\n"
    "    9: AND        r10, r9, r8\n"
    "   10: ANDI       r11, r10, 31\n"
    "   11: SGTUI      r12, r11, 0\n"
    "   12: SLLI       r13, r12, 30\n"
    "   13: SLLI       r14, r5, 29\n"
    "   14: OR         r15, r13, r14\n"
    "   15: OR         r16, r3, r15\n"
    "   16: ZCAST      r17, r16\n"
    "   17: BITCAST    r18, r17\n"
    "   18: BFEXT      r19, r3, 31, 1\n"
    "   19: BFEXT      r20, r3, 28, 1\n"
    "   20: GET_ALIAS  r21, a3\n"
    "   21: SLLI       r22, r19, 3\n"
    "   22: SLLI       r23, r12, 2\n"
    "   23: SLLI       r24, r5, 1\n"
    "   24: OR         r25, r22, r23\n"
    "   25: OR         r26, r24, r20\n"
    "   26: OR         r27, r25, r26\n"
    "   27: BFINS      r28, r21, r27, 24, 4\n"
    "   28: SET_ALIAS  a3, r28\n"
    "   29: SET_ALIAS  a2, r18\n"
    "   30: LOAD_IMM   r29, 4\n"
    "   31: SET_ALIAS  a1, r29\n"
    "   32: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
