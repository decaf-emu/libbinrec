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
    "   13: SLTSI      r9, r5, 0\n"
    "   14: SGTSI      r10, r5, 0\n"
    "   15: SEQI       r11, r5, 0\n"
    "   16: BFEXT      r12, r6, 31, 1\n"
    "   17: BFINS      r13, r12, r11, 1, 1\n"
    "   18: BFINS      r14, r13, r10, 2, 1\n"
    "   19: BFINS      r15, r14, r9, 3, 1\n"
    "   20: SET_ALIAS  a4, r15\n"
    "   21: LOAD_IMM   r16, 4\n"
    "   22: SET_ALIAS  a1, r16\n"
    "   23: LABEL      L2\n"
    "   24: GET_ALIAS  r17, a3\n"
    "   25: STORE      268(r1), r17\n"
    "   26: LOAD       r18, 928(r1)\n"
    "   27: ANDI       r19, r18, 268435455\n"
    "   28: GET_ALIAS  r20, a4\n"
    "   29: SLLI       r21, r20, 28\n"
    "   30: OR         r22, r19, r21\n"
    "   31: STORE      928(r1), r22\n"
    "   32: GET_ALIAS  r23, a5\n"
    "   33: STORE      940(r1), r23\n"
    "   34: GET_ALIAS  r24, a1\n"
    "   35: STORE      956(r1), r24\n"
    "   36: RETURN     r1\n"
    "\n"
    "Block    0: <none> --> [0,4] --> 1\n"
    "Block    1: 0 --> [5,22] --> 2\n"
    "Block    2: 1 --> [23,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
