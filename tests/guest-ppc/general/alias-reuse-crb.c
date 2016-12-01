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
    "   11: GET_ALIAS  r8, a8\n"
    "   12: BFEXT      r9, r8, 31, 1\n"
    "   13: SET_ALIAS  a9, r9\n"
    "   14: GET_ALIAS  r10, a2\n"
    "   15: SLTSI      r11, r10, 4660\n"
    "   16: SGTSI      r12, r10, 4660\n"
    "   17: SEQI       r13, r10, 4660\n"
    "   18: GET_ALIAS  r14, a9\n"
    "   19: SET_ALIAS  a4, r11\n"
    "   20: SET_ALIAS  a5, r12\n"
    "   21: SET_ALIAS  a6, r13\n"
    "   22: SET_ALIAS  a7, r14\n"
    "   23: GOTO_IF_NZ r13, L2\n"
    "   24: LOAD_IMM   r15, 8\n"
    "   25: SET_ALIAS  a1, r15\n"
    "   26: LABEL      L1\n"
    "   27: GOTO       L1\n"
    "   28: LOAD_IMM   r16, 12\n"
    "   29: SET_ALIAS  a1, r16\n"
    "   30: LABEL      L2\n"
    "   31: LOAD_IMM   r17, 16\n"
    "   32: SET_ALIAS  a1, r17\n"
    "   33: GET_ALIAS  r18, a3\n"
    "   34: ANDI       r19, r18, -16\n"
    "   35: GET_ALIAS  r20, a4\n"
    "   36: SLLI       r21, r20, 3\n"
    "   37: OR         r22, r19, r21\n"
    "   38: GET_ALIAS  r23, a5\n"
    "   39: SLLI       r24, r23, 2\n"
    "   40: OR         r25, r22, r24\n"
    "   41: GET_ALIAS  r26, a6\n"
    "   42: SLLI       r27, r26, 1\n"
    "   43: OR         r28, r25, r27\n"
    "   44: GET_ALIAS  r29, a7\n"
    "   45: OR         r30, r28, r29\n"
    "   46: SET_ALIAS  a3, r30\n"
    "   47: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 940(r1)\n"
    "Alias 9: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,23] --> 1,4\n"
    "Block 1: 0 --> [24,25] --> 2\n"
    "Block 2: 1,2 --> [26,27] --> 2\n"
    "Block 3: <none> --> [28,29] --> 4\n"
    "Block 4: 3,0 --> [30,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
