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
    0x70,0x64,0x00,0xFF,  // andi. r4,r3,255
    0x7C,0x6F,0xF1,0x20,  // mtcr r3
    0x41,0x82,0x00,0x08,  // beq 0x10
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 19\n"
        "[info] r10 no longer used, setting death = birth\n"
        "[info] Killing instruction 20\n"
        "[info] r11 no longer used, setting death = birth\n"
        "[info] Killing instruction 21\n"
        "[info] r12 no longer used, setting death = birth\n"
        "[info] Killing instruction 22\n"
        "[info] r14 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
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
    "   12: ANDI       r9, r8, 255\n"
    "   13: SET_ALIAS  a3, r9\n"
    "   14: SLTSI      r10, r9, 0\n"
    "   15: SGTSI      r11, r9, 0\n"
    "   16: SEQI       r12, r9, 0\n"
    "   17: GET_ALIAS  r13, a9\n"
    "   18: BFEXT      r14, r13, 31, 1\n"
    "   19: NOP\n"
    "   20: NOP\n"
    "   21: NOP\n"
    "   22: NOP\n"
    "   23: SET_ALIAS  a4, r8\n"
    "   24: BFEXT      r15, r8, 31, 1\n"
    "   25: BFEXT      r16, r8, 30, 1\n"
    "   26: BFEXT      r17, r8, 29, 1\n"
    "   27: BFEXT      r18, r8, 28, 1\n"
    "   28: SET_ALIAS  a5, r15\n"
    "   29: SET_ALIAS  a6, r16\n"
    "   30: SET_ALIAS  a7, r17\n"
    "   31: SET_ALIAS  a8, r18\n"
    "   32: GOTO_IF_Z  r17, L1\n"
    "   33: LOAD_IMM   r19, 16\n"
    "   34: SET_ALIAS  a1, r19\n"
    "   35: GOTO       L2\n"
    "   36: LABEL      L1\n"
    "   37: LOAD_IMM   r20, 12\n"
    "   38: SET_ALIAS  a1, r20\n"
    "   39: LABEL      L2\n"
    "   40: GET_ALIAS  r21, a4\n"
    "   41: ANDI       r22, r21, 268435455\n"
    "   42: GET_ALIAS  r23, a5\n"
    "   43: SLLI       r24, r23, 31\n"
    "   44: OR         r25, r22, r24\n"
    "   45: GET_ALIAS  r26, a6\n"
    "   46: SLLI       r27, r26, 30\n"
    "   47: OR         r28, r25, r27\n"
    "   48: GET_ALIAS  r29, a7\n"
    "   49: SLLI       r30, r29, 29\n"
    "   50: OR         r31, r28, r30\n"
    "   51: GET_ALIAS  r32, a8\n"
    "   52: SLLI       r33, r32, 28\n"
    "   53: OR         r34, r31, r33\n"
    "   54: SET_ALIAS  a4, r34\n"
    "   55: RETURN\n"
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
    "Block 0: <none> --> [0,32] --> 1,2\n"
    "Block 1: 0 --> [33,35] --> 3\n"
    "Block 2: 0 --> [36,38] --> 3\n"
    "Block 3: 2,1 --> [39,55] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
