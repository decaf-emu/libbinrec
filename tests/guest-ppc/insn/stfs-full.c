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
    0xD0,0x23,0xFF,0xF0,  // stfs f1,-16(r3)
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: GET_ALIAS  r6, a3\n"
    "    6: BITCAST    r7, r6\n"
    "    7: SRLI       r8, r7, 32\n"
    "    8: ZCAST      r9, r8\n"
    "    9: SLLI       r10, r7, 1\n"
    "   10: GOTO_IF_Z  r10, L1\n"
    "   11: LOAD_IMM   r11, 0x701FFFFFFFFFFFFF\n"
    "   12: SGTU       r12, r10, r11\n"
    "   13: GOTO_IF_Z  r12, L2\n"
    "   14: LABEL      L1\n"
    "   15: ANDI       r13, r9, -1073741824\n"
    "   16: BFEXT      r14, r7, 29, 30\n"
    "   17: ZCAST      r15, r14\n"
    "   18: OR         r16, r13, r15\n"
    "   19: STORE_BR   -16(r5), r16\n"
    "   20: GOTO       L3\n"
    "   21: LABEL      L2\n"
    "   22: SRLI       r17, r10, 53\n"
    "   23: SRLI       r18, r10, 31\n"
    "   24: ZCAST      r19, r18\n"
    "   25: LOAD_IMM   r20, 896\n"
    "   26: ANDI       r21, r19, 8388607\n"
    "   27: SUB        r22, r20, r17\n"
    "   28: ORI        r23, r21, 4194304\n"
    "   29: ANDI       r24, r9, -2147483648\n"
    "   30: SRL        r25, r23, r22\n"
    "   31: OR         r26, r24, r25\n"
    "   32: STORE_BR   -16(r5), r26\n"
    "   33: LABEL      L3\n"
    "   34: LOAD_IMM   r27, 4\n"
    "   35: SET_ALIAS  a1, r27\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64 @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,2\n"
    "Block 1: 0 --> [11,13] --> 2,3\n"
    "Block 2: 1,0 --> [14,20] --> 4\n"
    "Block 3: 1 --> [21,32] --> 4\n"
    "Block 4: 3,2 --> [33,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
