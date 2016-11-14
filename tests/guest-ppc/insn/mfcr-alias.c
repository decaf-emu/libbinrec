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
    0x7C,0x80,0x11,0x20,  // mtcrf 1,r4
    0x7C,0x60,0x00,0x26,  // mfcr r3
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: BFEXT      r9, r8, 3, 1\n"
    "   13: BFEXT      r10, r8, 2, 1\n"
    "   14: BFEXT      r11, r8, 1, 1\n"
    "   15: BFEXT      r12, r8, 0, 1\n"
    "   16: SET_ALIAS  a5, r9\n"
    "   17: SET_ALIAS  a6, r10\n"
    "   18: SET_ALIAS  a7, r11\n"
    "   19: SET_ALIAS  a8, r12\n"
    "   20: GET_ALIAS  r13, a4\n"
    "   21: ANDI       r14, r13, -16\n"
    "   22: SLLI       r15, r9, 3\n"
    "   23: OR         r16, r14, r15\n"
    "   24: SLLI       r17, r10, 2\n"
    "   25: OR         r18, r16, r17\n"
    "   26: SLLI       r19, r11, 1\n"
    "   27: OR         r20, r18, r19\n"
    "   28: OR         r21, r20, r12\n"
    "   29: SET_ALIAS  a4, r21\n"
    "   30: SET_ALIAS  a2, r21\n"
    "   31: LOAD_IMM   r22, 8\n"
    "   32: SET_ALIAS  a1, r22\n"
    "   33: GET_ALIAS  r23, a4\n"
    "   34: ANDI       r24, r23, -16\n"
    "   35: GET_ALIAS  r25, a5\n"
    "   36: SLLI       r26, r25, 3\n"
    "   37: OR         r27, r24, r26\n"
    "   38: GET_ALIAS  r28, a6\n"
    "   39: SLLI       r29, r28, 2\n"
    "   40: OR         r30, r27, r29\n"
    "   41: GET_ALIAS  r31, a7\n"
    "   42: SLLI       r32, r31, 1\n"
    "   43: OR         r33, r30, r32\n"
    "   44: GET_ALIAS  r34, a8\n"
    "   45: OR         r35, r33, r34\n"
    "   46: SET_ALIAS  a4, r35\n"
    "   47: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
