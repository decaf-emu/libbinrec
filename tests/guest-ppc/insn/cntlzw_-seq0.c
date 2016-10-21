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

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: CLZ        r4, r3\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: SLTSI      r5, r4, 0\n"
    "    6: SGTSI      r6, r4, 0\n"
    "    7: SEQI       r7, r4, 0\n"
    "    8: GET_ALIAS  r8, a9\n"
    "    9: BFEXT      r9, r8, 31, 1\n"
    "   10: SET_ALIAS  a4, r5\n"
    "   11: SET_ALIAS  a5, r6\n"
    "   12: SET_ALIAS  a6, r7\n"
    "   13: SET_ALIAS  a7, r9\n"
    "   14: SRLI       r10, r4, 5\n"
    "   15: SET_ALIAS  a2, r10\n"
    "   16: LOAD_IMM   r11, 8\n"
    "   17: SET_ALIAS  a1, r11\n"
    "   18: GET_ALIAS  r12, a8\n"
    "   19: ANDI       r13, r12, 268435455\n"
    "   20: GET_ALIAS  r14, a4\n"
    "   21: SLLI       r15, r14, 31\n"
    "   22: OR         r16, r13, r15\n"
    "   23: GET_ALIAS  r17, a5\n"
    "   24: SLLI       r18, r17, 30\n"
    "   25: OR         r19, r16, r18\n"
    "   26: GET_ALIAS  r20, a6\n"
    "   27: SLLI       r21, r20, 29\n"
    "   28: OR         r22, r19, r21\n"
    "   29: GET_ALIAS  r23, a7\n"
    "   30: SLLI       r24, r23, 28\n"
    "   31: OR         r25, r22, r24\n"
    "   32: SET_ALIAS  a8, r25\n"
    "   33: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,33] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
