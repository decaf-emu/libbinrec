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
    0x2F,0x80,0x12,0x34,  // cmpi cr7,r0,4660
    0x41,0x9E,0x00,0x08,  // beq cr7,0xC
    0x48,0x00,0x00,0x00,  // b 0x8
    0x60,0x00,0x00,0x00,  // nop
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xF\n"
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
    "   11: GET_ALIAS  r8, a2\n"
    "   12: SLTSI      r9, r8, 4660\n"
    "   13: SGTSI      r10, r8, 4660\n"
    "   14: SEQI       r11, r8, 4660\n"
    "   15: GET_ALIAS  r12, a8\n"
    "   16: BFEXT      r13, r12, 31, 1\n"
    "   17: SET_ALIAS  a4, r9\n"
    "   18: SET_ALIAS  a5, r10\n"
    "   19: SET_ALIAS  a6, r11\n"
    "   20: SET_ALIAS  a7, r13\n"
    "   21: GOTO_IF_NZ r11, L2\n"
    "   22: LOAD_IMM   r14, 8\n"
    "   23: SET_ALIAS  a1, r14\n"
    "   24: LABEL      L1\n"
    "   25: GOTO       L1\n"
    "   26: LOAD_IMM   r15, 12\n"
    "   27: SET_ALIAS  a1, r15\n"
    "   28: LABEL      L2\n"
    "   29: LOAD_IMM   r16, 16\n"
    "   30: SET_ALIAS  a1, r16\n"
    "   31: GET_ALIAS  r17, a3\n"
    "   32: ANDI       r18, r17, -16\n"
    "   33: GET_ALIAS  r19, a4\n"
    "   34: SLLI       r20, r19, 3\n"
    "   35: OR         r21, r18, r20\n"
    "   36: GET_ALIAS  r22, a5\n"
    "   37: SLLI       r23, r22, 2\n"
    "   38: OR         r24, r21, r23\n"
    "   39: GET_ALIAS  r25, a6\n"
    "   40: SLLI       r26, r25, 1\n"
    "   41: OR         r27, r24, r26\n"
    "   42: GET_ALIAS  r28, a7\n"
    "   43: OR         r29, r27, r28\n"
    "   44: SET_ALIAS  a3, r29\n"
    "   45: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> 1,4\n"
    "Block 1: 0 --> [22,23] --> 2\n"
    "Block 2: 1,2 --> [24,25] --> 2\n"
    "Block 3: <none> --> [26,27] --> 4\n"
    "Block 4: 3,0 --> [28,45] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
