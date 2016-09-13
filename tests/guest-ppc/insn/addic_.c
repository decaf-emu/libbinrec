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
    0x34, 0x60, 0x12, 0x34,  // addic. r3,r0,4660
};

static const bool expected_success = true;

static const char expected[] =
    "[warning] Scanning terminated at 0x4 due to code range limit\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD       r2, 256(r1)\n"
    "    2: SET_ALIAS  a2, r2\n"
    "    3: LOAD       r3, 940(r1)\n"
    "    4: SET_ALIAS  a5, r3\n"
    "    5: LABEL      L1\n"
    "    6: GET_ALIAS  r4, a2\n"
    "    7: LOAD_IMM   r5, 4660\n"
    "    8: ADD        r6, r5, r4\n"
    "    9: SET_ALIAS  a3, r6\n"
    "   10: GET_ALIAS  r7, a5\n"
    "   11: SLTU       r8, r6, r5\n"
    "   12: BFINS      r9, r7, r8, 29, 1\n"
    "   13: SET_ALIAS  a5, r9\n"
    "   14: LOAD_IMM   r10, 0\n"
    "   15: SLTS       r11, r6, r10\n"
    "   16: SLTS       r12, r10, r6\n"
    "   17: SEQ        r13, r6, r10\n"
    "   18: BFEXT      r14, r7, 31, 1\n"
    "   19: BFINS      r15, r14, r13, 1, 1\n"
    "   20: BFINS      r16, r15, r12, 2, 1\n"
    "   21: BFINS      r17, r16, r11, 3, 1\n"
    "   22: SET_ALIAS  a4, r17\n"
    "   23: LOAD_IMM   r18, 4\n"
    "   24: SET_ALIAS  a1, r18\n"
    "   25: LABEL      L2\n"
    "   26: GET_ALIAS  r19, a3\n"
    "   27: STORE      268(r1), r19\n"
    "   28: LOAD       r20, 928(r1)\n"
    "   29: LOAD_IMM   r21, 0xFFFFFFF\n"
    "   30: AND        r22, r20, r21\n"
    "   31: GET_ALIAS  r23, a4\n"
    "   32: LOAD_IMM   r24, 28\n"
    "   33: SLL        r25, r23, r24\n"
    "   34: OR         r26, r22, r25\n"
    "   35: STORE      928(r1), r26\n"
    "   36: GET_ALIAS  r27, a5\n"
    "   37: STORE      940(r1), r27\n"
    "   38: GET_ALIAS  r28, a1\n"
    "   39: STORE      956(r1), r28\n"
    "   40: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,4] --> 1\n"
    "Block    1: 0 --> [5,24] --> 2\n"
    "Block    2: 1 --> [25,40] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
