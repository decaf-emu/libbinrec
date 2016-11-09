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
    0x7C,0x64,0x00,0x35,  // cntlzw. r4,r3 (should not be optimized since Rc=1)
    0x54,0x83,0xD9,0x7E,  // rlwinm r3,r4,27,5,31 (srwi r3,r4,5)
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
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
    "   12: CLZ        r9, r8\n"
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
    "   23: SRLI       r15, r9, 5\n"
    "   24: SET_ALIAS  a2, r15\n"
    "   25: LOAD_IMM   r16, 8\n"
    "   26: SET_ALIAS  a1, r16\n"
    "   27: GET_ALIAS  r17, a4\n"
    "   28: ANDI       r18, r17, 268435455\n"
    "   29: GET_ALIAS  r19, a5\n"
    "   30: SLLI       r20, r19, 31\n"
    "   31: OR         r21, r18, r20\n"
    "   32: GET_ALIAS  r22, a6\n"
    "   33: SLLI       r23, r22, 30\n"
    "   34: OR         r24, r21, r23\n"
    "   35: GET_ALIAS  r25, a7\n"
    "   36: SLLI       r26, r25, 29\n"
    "   37: OR         r27, r24, r26\n"
    "   38: GET_ALIAS  r28, a8\n"
    "   39: SLLI       r29, r28, 28\n"
    "   40: OR         r30, r27, r29\n"
    "   41: SET_ALIAS  a4, r30\n"
    "   42: RETURN\n"
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
    "Block 0: <none> --> [0,42] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
