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

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a7\n"
    "    3: SET_ALIAS  a2, r3\n"
    "    4: SLTSI      r4, r3, 4660\n"
    "    5: SGTSI      r5, r3, 4660\n"
    "    6: SEQI       r6, r3, 4660\n"
    "    7: GET_ALIAS  r7, a8\n"
    "    8: BFEXT      r8, r7, 31, 1\n"
    "    9: SET_ALIAS  a3, r4\n"
    "   10: SET_ALIAS  a4, r5\n"
    "   11: SET_ALIAS  a5, r6\n"
    "   12: SET_ALIAS  a6, r8\n"
    "   13: LOAD_IMM   r9, 0\n"
    "   14: SLTS       r10, r3, r9\n"
    "   15: GOTO_IF_Z  r10, L1\n"
    "   16: ANDI       r11, r3, -16\n"
    "   17: SLLI       r12, r4, 3\n"
    "   18: OR         r13, r11, r12\n"
    "   19: SLLI       r14, r5, 2\n"
    "   20: OR         r15, r13, r14\n"
    "   21: SLLI       r16, r6, 1\n"
    "   22: OR         r17, r15, r16\n"
    "   23: OR         r18, r17, r8\n"
    "   24: SET_ALIAS  a7, r18\n"
    "   25: LOAD_IMM   r19, 8\n"
    "   26: SET_ALIAS  a1, r19\n"
    "   27: LOAD       r20, 984(r1)\n"
    "   28: CALL       @r20, r1\n"
    "   29: RETURN\n"
    "   30: LABEL      L1\n"
    "   31: LOAD_IMM   r21, 12\n"
    "   32: SET_ALIAS  a1, r21\n"
    "   33: GET_ALIAS  r22, a7\n"
    "   34: ANDI       r23, r22, -16\n"
    "   35: GET_ALIAS  r24, a3\n"
    "   36: SLLI       r25, r24, 3\n"
    "   37: OR         r26, r23, r25\n"
    "   38: GET_ALIAS  r27, a4\n"
    "   39: SLLI       r28, r27, 2\n"
    "   40: OR         r29, r26, r28\n"
    "   41: GET_ALIAS  r30, a5\n"
    "   42: SLLI       r31, r30, 1\n"
    "   43: OR         r32, r29, r31\n"
    "   44: GET_ALIAS  r33, a6\n"
    "   45: OR         r34, r32, r33\n"
    "   46: SET_ALIAS  a7, r34\n"
    "   47: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 928(r1)\n"
    "Alias 8: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,15] --> 1,2\n"
    "Block 1: 0 --> [16,29] --> <none>\n"
    "Block 2: 0 --> [30,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
