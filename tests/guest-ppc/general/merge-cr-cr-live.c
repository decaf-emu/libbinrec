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
    0x7C,0x60,0x00,0x26,  // mfcr r3
    0x2F,0x83,0x12,0x34,  // cmpi cr7,r3,4660
    0x0E,0x03,0x00,0x00,  // twlti r3,0
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a5, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a6, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a7, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: ANDI       r9, r8, -16\n"
    "   13: GET_ALIAS  r10, a4\n"
    "   14: SLLI       r11, r10, 3\n"
    "   15: OR         r12, r9, r11\n"
    "   16: GET_ALIAS  r13, a5\n"
    "   17: SLLI       r14, r13, 2\n"
    "   18: OR         r15, r12, r14\n"
    "   19: GET_ALIAS  r16, a6\n"
    "   20: SLLI       r17, r16, 1\n"
    "   21: OR         r18, r15, r17\n"
    "   22: GET_ALIAS  r19, a7\n"
    "   23: OR         r20, r18, r19\n"
    "   24: SET_ALIAS  a3, r20\n"
    "   25: SET_ALIAS  a2, r20\n"
    "   26: SLTSI      r21, r20, 4660\n"
    "   27: SGTSI      r22, r20, 4660\n"
    "   28: SEQI       r23, r20, 4660\n"
    "   29: GET_ALIAS  r24, a8\n"
    "   30: BFEXT      r25, r24, 31, 1\n"
    "   31: SET_ALIAS  a4, r21\n"
    "   32: SET_ALIAS  a5, r22\n"
    "   33: SET_ALIAS  a6, r23\n"
    "   34: SET_ALIAS  a7, r25\n"
    "   35: LOAD_IMM   r26, 0\n"
    "   36: SLTS       r27, r20, r26\n"
    "   37: GOTO_IF_Z  r27, L1\n"
    "   38: ANDI       r28, r20, -16\n"
    "   39: SLLI       r29, r21, 3\n"
    "   40: OR         r30, r28, r29\n"
    "   41: SLLI       r31, r22, 2\n"
    "   42: OR         r32, r30, r31\n"
    "   43: SLLI       r33, r23, 1\n"
    "   44: OR         r34, r32, r33\n"
    "   45: OR         r35, r34, r25\n"
    "   46: SET_ALIAS  a3, r35\n"
    "   47: LOAD_IMM   r36, 8\n"
    "   48: SET_ALIAS  a1, r36\n"
    "   49: LOAD       r37, 984(r1)\n"
    "   50: CALL       @r37, r1\n"
    "   51: RETURN\n"
    "   52: LABEL      L1\n"
    "   53: LOAD_IMM   r38, 12\n"
    "   54: SET_ALIAS  a1, r38\n"
    "   55: GET_ALIAS  r39, a3\n"
    "   56: ANDI       r40, r39, -16\n"
    "   57: GET_ALIAS  r41, a4\n"
    "   58: SLLI       r42, r41, 3\n"
    "   59: OR         r43, r40, r42\n"
    "   60: GET_ALIAS  r44, a5\n"
    "   61: SLLI       r45, r44, 2\n"
    "   62: OR         r46, r43, r45\n"
    "   63: GET_ALIAS  r47, a6\n"
    "   64: SLLI       r48, r47, 1\n"
    "   65: OR         r49, r46, r48\n"
    "   66: GET_ALIAS  r50, a7\n"
    "   67: OR         r51, r49, r50\n"
    "   68: SET_ALIAS  a3, r51\n"
    "   69: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,37] --> 1,2\n"
    "Block 1: 0 --> [38,51] --> <none>\n"
    "Block 2: 0 --> [52,69] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
