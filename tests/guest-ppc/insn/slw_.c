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
    0x7C,0x83,0x28,0x31,  // slw. r3,r4,r5
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a5\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: SET_ALIAS  a6, r4\n"
    "    5: BFEXT      r5, r3, 30, 1\n"
    "    6: SET_ALIAS  a7, r5\n"
    "    7: BFEXT      r6, r3, 29, 1\n"
    "    8: SET_ALIAS  a8, r6\n"
    "    9: BFEXT      r7, r3, 28, 1\n"
    "   10: SET_ALIAS  a9, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: GET_ALIAS  r9, a4\n"
    "   13: ANDI       r10, r9, 63\n"
    "   14: ZCAST      r11, r8\n"
    "   15: SLL        r12, r11, r10\n"
    "   16: ZCAST      r13, r12\n"
    "   17: SET_ALIAS  a2, r13\n"
    "   18: SLTSI      r14, r13, 0\n"
    "   19: SGTSI      r15, r13, 0\n"
    "   20: SEQI       r16, r13, 0\n"
    "   21: GET_ALIAS  r17, a10\n"
    "   22: BFEXT      r18, r17, 31, 1\n"
    "   23: SET_ALIAS  a6, r14\n"
    "   24: SET_ALIAS  a7, r15\n"
    "   25: SET_ALIAS  a8, r16\n"
    "   26: SET_ALIAS  a9, r18\n"
    "   27: LOAD_IMM   r19, 4\n"
    "   28: SET_ALIAS  a1, r19\n"
    "   29: GET_ALIAS  r20, a5\n"
    "   30: ANDI       r21, r20, 268435455\n"
    "   31: GET_ALIAS  r22, a6\n"
    "   32: SLLI       r23, r22, 31\n"
    "   33: OR         r24, r21, r23\n"
    "   34: GET_ALIAS  r25, a7\n"
    "   35: SLLI       r26, r25, 30\n"
    "   36: OR         r27, r24, r26\n"
    "   37: GET_ALIAS  r28, a8\n"
    "   38: SLLI       r29, r28, 29\n"
    "   39: OR         r30, r27, r29\n"
    "   40: GET_ALIAS  r31, a9\n"
    "   41: SLLI       r32, r31, 28\n"
    "   42: OR         r33, r30, r32\n"
    "   43: SET_ALIAS  a5, r33\n"
    "   44: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,44] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
