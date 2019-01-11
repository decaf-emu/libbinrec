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
    0xFC,0x00,0x0D,0x8F,  // mtfsf. 0,f1
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BITCAST    r4, r3\n"
    "    4: ZCAST      r5, r4\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: ANDI       r7, r6, 33031936\n"
    "    7: SGTUI      r8, r7, 0\n"
    "    8: BFEXT      r9, r6, 25, 4\n"
    "    9: SLLI       r10, r8, 4\n"
    "   10: SRLI       r11, r6, 3\n"
    "   11: OR         r12, r9, r10\n"
    "   12: AND        r13, r12, r11\n"
    "   13: ANDI       r14, r13, 31\n"
    "   14: SGTUI      r15, r14, 0\n"
    "   15: BFEXT      r16, r6, 31, 1\n"
    "   16: BFEXT      r17, r6, 28, 1\n"
    "   17: GET_ALIAS  r18, a3\n"
    "   18: SLLI       r19, r16, 3\n"
    "   19: SLLI       r20, r15, 2\n"
    "   20: SLLI       r21, r8, 1\n"
    "   21: OR         r22, r19, r20\n"
    "   22: OR         r23, r21, r17\n"
    "   23: OR         r24, r22, r23\n"
    "   24: BFINS      r25, r18, r24, 24, 4\n"
    "   25: SET_ALIAS  a3, r25\n"
    "   26: LOAD_IMM   r26, 4\n"
    "   27: SET_ALIAS  a1, r26\n"
    "   28: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
