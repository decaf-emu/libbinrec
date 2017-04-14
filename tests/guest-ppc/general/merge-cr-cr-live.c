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

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
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
    "   11: GET_ALIAS  r8, a8\n"
    "   12: BFEXT      r9, r8, 31, 1\n"
    "   13: SET_ALIAS  a9, r9\n"
    "   14: GET_ALIAS  r10, a3\n"
    "   15: ANDI       r11, r10, -16\n"
    "   16: GET_ALIAS  r12, a4\n"
    "   17: SLLI       r13, r12, 3\n"
    "   18: OR         r14, r11, r13\n"
    "   19: GET_ALIAS  r15, a5\n"
    "   20: SLLI       r16, r15, 2\n"
    "   21: OR         r17, r14, r16\n"
    "   22: GET_ALIAS  r18, a6\n"
    "   23: SLLI       r19, r18, 1\n"
    "   24: OR         r20, r17, r19\n"
    "   25: GET_ALIAS  r21, a7\n"
    "   26: OR         r22, r20, r21\n"
    "   27: SET_ALIAS  a3, r22\n"
    "   28: SET_ALIAS  a2, r22\n"
    "   29: SLTSI      r23, r22, 4660\n"
    "   30: SGTSI      r24, r22, 4660\n"
    "   31: SEQI       r25, r22, 4660\n"
    "   32: GET_ALIAS  r26, a9\n"
    "   33: SET_ALIAS  a4, r23\n"
    "   34: SET_ALIAS  a5, r24\n"
    "   35: SET_ALIAS  a6, r25\n"
    "   36: SET_ALIAS  a7, r26\n"
    "   37: LOAD_IMM   r27, 0\n"
    "   38: SLTS       r28, r22, r27\n"
    "   39: GOTO_IF_Z  r28, L1\n"
    "   40: ANDI       r29, r22, -16\n"
    "   41: SLLI       r30, r23, 3\n"
    "   42: OR         r31, r29, r30\n"
    "   43: SLLI       r32, r24, 2\n"
    "   44: OR         r33, r31, r32\n"
    "   45: SLLI       r34, r25, 1\n"
    "   46: OR         r35, r33, r34\n"
    "   47: OR         r36, r35, r26\n"
    "   48: SET_ALIAS  a3, r36\n"
    "   49: LOAD_IMM   r37, 8\n"
    "   50: SET_ALIAS  a1, r37\n"
    "   51: LOAD       r38, 992(r1)\n"
    "   52: CALL       @r38, r1\n"
    "   53: RETURN\n"
    "   54: LABEL      L1\n"
    "   55: LOAD_IMM   r39, 12\n"
    "   56: SET_ALIAS  a1, r39\n"
    "   57: GET_ALIAS  r40, a3\n"
    "   58: ANDI       r41, r40, -16\n"
    "   59: GET_ALIAS  r42, a4\n"
    "   60: SLLI       r43, r42, 3\n"
    "   61: OR         r44, r41, r43\n"
    "   62: GET_ALIAS  r45, a5\n"
    "   63: SLLI       r46, r45, 2\n"
    "   64: OR         r47, r44, r46\n"
    "   65: GET_ALIAS  r48, a6\n"
    "   66: SLLI       r49, r48, 1\n"
    "   67: OR         r50, r47, r49\n"
    "   68: GET_ALIAS  r51, a7\n"
    "   69: OR         r52, r50, r51\n"
    "   70: SET_ALIAS  a3, r52\n"
    "   71: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 940(r1)\n"
    "Alias 9: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,39] --> 1,2\n"
    "Block 1: 0 --> [40,53] --> <none>\n"
    "Block 2: 0 --> [54,71] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
