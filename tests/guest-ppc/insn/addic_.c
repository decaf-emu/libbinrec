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
    0x34,0x60,0x12,0x34,  // addic. r3,r0,4660
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 30, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 29, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 28, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a2\n"
    "   12: ADDI       r9, r8, 4660\n"
    "   13: SET_ALIAS  a3, r9\n"
    "   14: GET_ALIAS  r10, a9\n"
    "   15: SLTUI      r11, r9, 4660\n"
    "   16: BFINS      r12, r10, r11, 29, 1\n"
    "   17: SET_ALIAS  a9, r12\n"
    "   18: SLTSI      r13, r9, 0\n"
    "   19: SGTSI      r14, r9, 0\n"
    "   20: SEQI       r15, r9, 0\n"
    "   21: BFEXT      r16, r12, 31, 1\n"
    "   22: SET_ALIAS  a5, r13\n"
    "   23: SET_ALIAS  a6, r14\n"
    "   24: SET_ALIAS  a7, r15\n"
    "   25: SET_ALIAS  a8, r16\n"
    "   26: LOAD_IMM   r17, 4\n"
    "   27: SET_ALIAS  a1, r17\n"
    "   28: GET_ALIAS  r18, a4\n"
    "   29: ANDI       r19, r18, 268435455\n"
    "   30: GET_ALIAS  r20, a5\n"
    "   31: SLLI       r21, r20, 31\n"
    "   32: OR         r22, r19, r21\n"
    "   33: GET_ALIAS  r23, a6\n"
    "   34: SLLI       r24, r23, 30\n"
    "   35: OR         r25, r22, r24\n"
    "   36: GET_ALIAS  r26, a7\n"
    "   37: SLLI       r27, r26, 29\n"
    "   38: OR         r28, r25, r27\n"
    "   39: GET_ALIAS  r29, a8\n"
    "   40: SLLI       r30, r29, 28\n"
    "   41: OR         r31, r28, r30\n"
    "   42: SET_ALIAS  a4, r31\n"
    "   43: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 268(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,43] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
