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
    0x7C,0x8F,0x01,0x20,  // mtcrf 240,r4
    0x7C,0xA0,0xF1,0x20,  // mtcrf 15,r5
    0x7C,0x60,0x00,0x26,  // mfcr r3
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LABEL      L1\n"
    "    3: GET_ALIAS  r3, a3\n"
    "    4: BFEXT      r4, r3, 28, 4\n"
    "    5: BFEXT      r5, r3, 24, 4\n"
    "    6: BFEXT      r6, r3, 20, 4\n"
    "    7: BFEXT      r7, r3, 16, 4\n"
    "    8: GET_ALIAS  r8, a4\n"
    "    9: BFEXT      r9, r8, 12, 4\n"
    "   10: BFEXT      r10, r8, 8, 4\n"
    "   11: BFEXT      r11, r8, 4, 4\n"
    "   12: BFEXT      r12, r8, 0, 4\n"
    "   13: SLLI       r13, r4, 28\n"
    "   14: SLLI       r14, r5, 24\n"
    "   15: OR         r15, r13, r14\n"
    "   16: SLLI       r16, r6, 20\n"
    "   17: OR         r17, r15, r16\n"
    "   18: SLLI       r18, r7, 16\n"
    "   19: OR         r19, r17, r18\n"
    "   20: SLLI       r20, r9, 12\n"
    "   21: OR         r21, r19, r20\n"
    "   22: SLLI       r22, r10, 8\n"
    "   23: OR         r23, r21, r22\n"
    "   24: SLLI       r24, r11, 4\n"
    "   25: OR         r25, r23, r24\n"
    "   26: OR         r26, r25, r12\n"
    "   27: SET_ALIAS  a2, r26\n"
    "   28: SET_ALIAS  a5, r4\n"
    "   29: SET_ALIAS  a6, r5\n"
    "   30: SET_ALIAS  a7, r6\n"
    "   31: SET_ALIAS  a8, r7\n"
    "   32: SET_ALIAS  a9, r9\n"
    "   33: SET_ALIAS  a10, r10\n"
    "   34: SET_ALIAS  a11, r11\n"
    "   35: SET_ALIAS  a12, r12\n"
    "   36: LOAD_IMM   r27, 12\n"
    "   37: SET_ALIAS  a1, r27\n"
    "   38: LABEL      L2\n"
    "   39: GET_ALIAS  r28, a5\n"
    "   40: SLLI       r29, r28, 28\n"
    "   41: GET_ALIAS  r30, a6\n"
    "   42: SLLI       r31, r30, 24\n"
    "   43: OR         r32, r29, r31\n"
    "   44: GET_ALIAS  r33, a7\n"
    "   45: SLLI       r34, r33, 20\n"
    "   46: OR         r35, r32, r34\n"
    "   47: GET_ALIAS  r36, a8\n"
    "   48: SLLI       r37, r36, 16\n"
    "   49: OR         r38, r35, r37\n"
    "   50: GET_ALIAS  r39, a9\n"
    "   51: SLLI       r40, r39, 12\n"
    "   52: OR         r41, r38, r40\n"
    "   53: GET_ALIAS  r42, a10\n"
    "   54: SLLI       r43, r42, 8\n"
    "   55: OR         r44, r41, r43\n"
    "   56: GET_ALIAS  r45, a11\n"
    "   57: SLLI       r46, r45, 4\n"
    "   58: OR         r47, r44, r46\n"
    "   59: GET_ALIAS  r48, a12\n"
    "   60: OR         r49, r47, r48\n"
    "   61: STORE      928(r1), r49\n"
    "   62: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32, no bound storage\n"
    "Alias 11: int32, no bound storage\n"
    "Alias 12: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,1] --> 1\n"
    "Block 1: 0 --> [2,37] --> 2\n"
    "Block 2: 1 --> [38,62] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
