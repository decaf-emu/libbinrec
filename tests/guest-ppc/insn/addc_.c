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
    0x7C,0x64,0x28,0x15,  // addc. r3,r4,r5
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

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
    "   13: ADD        r10, r8, r9\n"
    "   14: SET_ALIAS  a2, r10\n"
    "   15: SRLI       r11, r8, 31\n"
    "   16: SRLI       r12, r9, 31\n"
    "   17: XOR        r13, r11, r12\n"
    "   18: SRLI       r14, r10, 31\n"
    "   19: XORI       r15, r14, 1\n"
    "   20: AND        r16, r11, r12\n"
    "   21: AND        r17, r13, r15\n"
    "   22: OR         r18, r16, r17\n"
    "   23: GET_ALIAS  r19, a10\n"
    "   24: BFINS      r20, r19, r18, 29, 1\n"
    "   25: SET_ALIAS  a10, r20\n"
    "   26: SLTSI      r21, r10, 0\n"
    "   27: SGTSI      r22, r10, 0\n"
    "   28: SEQI       r23, r10, 0\n"
    "   29: BFEXT      r24, r20, 31, 1\n"
    "   30: SET_ALIAS  a6, r21\n"
    "   31: SET_ALIAS  a7, r22\n"
    "   32: SET_ALIAS  a8, r23\n"
    "   33: SET_ALIAS  a9, r24\n"
    "   34: LOAD_IMM   r25, 4\n"
    "   35: SET_ALIAS  a1, r25\n"
    "   36: GET_ALIAS  r26, a5\n"
    "   37: ANDI       r27, r26, 268435455\n"
    "   38: GET_ALIAS  r28, a6\n"
    "   39: SLLI       r29, r28, 31\n"
    "   40: OR         r30, r27, r29\n"
    "   41: GET_ALIAS  r31, a7\n"
    "   42: SLLI       r32, r31, 30\n"
    "   43: OR         r33, r30, r32\n"
    "   44: GET_ALIAS  r34, a8\n"
    "   45: SLLI       r35, r34, 29\n"
    "   46: OR         r36, r33, r35\n"
    "   47: GET_ALIAS  r37, a9\n"
    "   48: SLLI       r38, r37, 28\n"
    "   49: OR         r39, r36, r38\n"
    "   50: SET_ALIAS  a5, r39\n"
    "   51: RETURN\n"
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
    "Block 0: <none> --> [0,51] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
