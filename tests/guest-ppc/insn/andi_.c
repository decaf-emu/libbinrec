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
    0x70,0x03,0xAB,0xCD,  // andi. r3,r0,43981
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
    "   12: ANDI       r9, r8, 43981\n"
    "   13: SET_ALIAS  a3, r9\n"
    "   14: SLTSI      r10, r9, 0\n"
    "   15: SGTSI      r11, r9, 0\n"
    "   16: SEQI       r12, r9, 0\n"
    "   17: GET_ALIAS  r13, a9\n"
    "   18: BFEXT      r14, r13, 31, 1\n"
    "   19: SET_ALIAS  a5, r10\n"
    "   20: SET_ALIAS  a6, r11\n"
    "   21: SET_ALIAS  a7, r12\n"
    "   22: SET_ALIAS  a8, r14\n"
    "   23: LOAD_IMM   r15, 4\n"
    "   24: SET_ALIAS  a1, r15\n"
    "   25: GET_ALIAS  r16, a4\n"
    "   26: ANDI       r17, r16, 268435455\n"
    "   27: GET_ALIAS  r18, a5\n"
    "   28: SLLI       r19, r18, 31\n"
    "   29: OR         r20, r17, r19\n"
    "   30: GET_ALIAS  r21, a6\n"
    "   31: SLLI       r22, r21, 30\n"
    "   32: OR         r23, r20, r22\n"
    "   33: GET_ALIAS  r24, a7\n"
    "   34: SLLI       r25, r24, 29\n"
    "   35: OR         r26, r23, r25\n"
    "   36: GET_ALIAS  r27, a8\n"
    "   37: SLLI       r28, r27, 28\n"
    "   38: OR         r29, r26, r28\n"
    "   39: SET_ALIAS  a4, r29\n"
    "   40: RETURN\n"
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
    "Block 0: <none> --> [0,40] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
