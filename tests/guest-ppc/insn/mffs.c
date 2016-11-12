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
    0xFC,0x20,0x04,0x8E,  // mffs f1
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a3\n"
    "    6: ANDI       r6, r5, 33031936\n"
    "    7: SGTUI      r7, r6, 0\n"
    "    8: BFEXT      r8, r5, 25, 4\n"
    "    9: SLLI       r9, r7, 4\n"
    "   10: SRLI       r10, r5, 3\n"
    "   11: OR         r11, r8, r9\n"
    "   12: AND        r12, r11, r10\n"
    "   13: ANDI       r13, r12, 31\n"
    "   14: SGTUI      r14, r13, 0\n"
    "   15: GET_ALIAS  r15, a4\n"
    "   16: ANDI       r16, r5, -1611134977\n"
    "   17: SLLI       r17, r15, 12\n"
    "   18: OR         r18, r16, r17\n"
    "   19: SLLI       r19, r14, 30\n"
    "   20: SLLI       r20, r7, 29\n"
    "   21: OR         r21, r19, r20\n"
    "   22: OR         r22, r18, r21\n"
    "   23: ZCAST      r23, r22\n"
    "   24: BITCAST    r24, r23\n"
    "   25: SET_ALIAS  a2, r24\n"
    "   26: LOAD_IMM   r25, 4\n"
    "   27: SET_ALIAS  a1, r25\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
