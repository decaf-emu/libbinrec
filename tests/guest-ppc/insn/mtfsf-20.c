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
    0xFC,0x40,0x0D,0x8E,  // mtfsf 32,f1
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: GET_ALIAS  r8, a3\n"
    "    9: ANDI       r9, r7, 15728640\n"
    "   10: ANDI       r10, r8, -1626341377\n"
    "   11: OR         r11, r10, r9\n"
    "   12: ANDI       r12, r11, 33031936\n"
    "   13: SGTUI      r13, r12, 0\n"
    "   14: SLLI       r14, r13, 29\n"
    "   15: OR         r15, r11, r14\n"
    "   16: SRLI       r16, r15, 25\n"
    "   17: SRLI       r17, r15, 3\n"
    "   18: AND        r18, r16, r17\n"
    "   19: ANDI       r19, r18, 31\n"
    "   20: SGTUI      r20, r19, 0\n"
    "   21: SLLI       r21, r20, 30\n"
    "   22: OR         r22, r15, r21\n"
    "   23: SET_ALIAS  a3, r22\n"
    "   24: LOAD_IMM   r23, 4\n"
    "   25: SET_ALIAS  a1, r23\n"
    "   26: GET_ALIAS  r24, a3\n"
    "   27: GET_ALIAS  r25, a4\n"
    "   28: ANDI       r26, r24, -522241\n"
    "   29: SLLI       r27, r25, 12\n"
    "   30: OR         r28, r26, r27\n"
    "   31: SET_ALIAS  a3, r28\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
