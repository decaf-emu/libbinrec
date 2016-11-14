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
    0x54,0x83,0x29,0x8F,  // rlwinm. r3,r4,5,6,7
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 31, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 30, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 29, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 28, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: RORI       r9, r8, 27\n"
    "   13: ANDI       r10, r9, 50331648\n"
    "   14: SET_ALIAS  a2, r10\n"
    "   15: SLTSI      r11, r10, 0\n"
    "   16: SGTSI      r12, r10, 0\n"
    "   17: SEQI       r13, r10, 0\n"
    "   18: GET_ALIAS  r14, a9\n"
    "   19: BFEXT      r15, r14, 31, 1\n"
    "   20: SET_ALIAS  a5, r11\n"
    "   21: SET_ALIAS  a6, r12\n"
    "   22: SET_ALIAS  a7, r13\n"
    "   23: SET_ALIAS  a8, r15\n"
    "   24: LOAD_IMM   r16, 4\n"
    "   25: SET_ALIAS  a1, r16\n"
    "   26: GET_ALIAS  r17, a4\n"
    "   27: ANDI       r18, r17, 268435455\n"
    "   28: GET_ALIAS  r19, a5\n"
    "   29: SLLI       r20, r19, 31\n"
    "   30: OR         r21, r18, r20\n"
    "   31: GET_ALIAS  r22, a6\n"
    "   32: SLLI       r23, r22, 30\n"
    "   33: OR         r24, r21, r23\n"
    "   34: GET_ALIAS  r25, a7\n"
    "   35: SLLI       r26, r25, 29\n"
    "   36: OR         r27, r24, r26\n"
    "   37: GET_ALIAS  r28, a8\n"
    "   38: SLLI       r29, r28, 28\n"
    "   39: OR         r30, r27, r29\n"
    "   40: SET_ALIAS  a4, r30\n"
    "   41: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,41] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
