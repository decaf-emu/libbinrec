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
    "    7: ADDI       r5, r4, 4660\n"
    "    8: SET_ALIAS  a3, r5\n"
    "    9: GET_ALIAS  r6, a5\n"
    "   10: SLTUI      r7, r5, 4660\n"
    "   11: BFINS      r8, r6, r7, 29, 1\n"
    "   12: SET_ALIAS  a5, r8\n"
    "   13: LOAD_IMM   r9, 0\n"
    "   14: SLTS       r10, r5, r9\n"
    "   15: SLTS       r11, r9, r5\n"
    "   16: SEQ        r12, r5, r9\n"
    "   17: BFEXT      r13, r6, 31, 1\n"
    "   18: BFINS      r14, r13, r12, 1, 1\n"
    "   19: BFINS      r15, r14, r11, 2, 1\n"
    "   20: BFINS      r16, r15, r10, 3, 1\n"
    "   21: SET_ALIAS  a4, r16\n"
    "   22: LOAD_IMM   r17, 4\n"
    "   23: SET_ALIAS  a1, r17\n"
    "   24: LABEL      L2\n"
    "   25: GET_ALIAS  r18, a3\n"
    "   26: STORE      268(r1), r18\n"
    "   27: LOAD       r19, 928(r1)\n"
    "   28: ANDI       r20, r19, 268435455\n"
    "   29: GET_ALIAS  r21, a4\n"
    "   30: SLLI       r22, r21, 28\n"
    "   31: OR         r23, r20, r22\n"
    "   32: STORE      928(r1), r23\n"
    "   33: GET_ALIAS  r24, a5\n"
    "   34: STORE      940(r1), r24\n"
    "   35: GET_ALIAS  r25, a1\n"
    "   36: STORE      956(r1), r25\n"
    "   37: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,4] --> 1\n"
    "Block    1: 0 --> [5,23] --> 2\n"
    "Block    2: 1 --> [24,37] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
