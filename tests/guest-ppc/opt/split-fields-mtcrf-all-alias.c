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
        "[info] Killing instruction 21\n"
        "[info] r12 no longer used, setting death = birth\n"
        "[info] Killing instruction 22\n"
        "[info] r13 no longer used, setting death = birth\n"
        "[info] Killing instruction 23\n"
        "[info] r14 no longer used, setting death = birth\n"
        "[info] Killing instruction 24\n"
        "[info] r15 no longer used, setting death = birth\n"
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
    "   11: GET_ALIAS  r8, a9\n"
    "   12: BFEXT      r9, r8, 31, 1\n"
    "   13: SET_ALIAS  a10, r9\n"
    "   14: GET_ALIAS  r10, a2\n"
    "   15: ANDI       r11, r10, 255\n"
    "   16: SET_ALIAS  a3, r11\n"
    "   17: SLTSI      r12, r11, 0\n"
    "   18: SGTSI      r13, r11, 0\n"
    "   19: SEQI       r14, r11, 0\n"
    "   20: GET_ALIAS  r15, a10\n"
    "   21: NOP\n"
    "   22: NOP\n"
    "   23: NOP\n"
    "   24: NOP\n"
    "   25: SET_ALIAS  a4, r10\n"
    "   26: BFEXT      r16, r10, 31, 1\n"
    "   27: BFEXT      r17, r10, 30, 1\n"
    "   28: BFEXT      r18, r10, 29, 1\n"
    "   29: BFEXT      r19, r10, 28, 1\n"
    "   30: SET_ALIAS  a5, r16\n"
    "   31: SET_ALIAS  a6, r17\n"
    "   32: SET_ALIAS  a7, r18\n"
    "   33: SET_ALIAS  a8, r19\n"
    "   34: GOTO_IF_Z  r18, L1\n"
    "   35: LOAD_IMM   r20, 16\n"
    "   36: SET_ALIAS  a1, r20\n"
    "   37: GOTO       L2\n"
    "   38: LABEL      L1\n"
    "   39: LOAD_IMM   r21, 12\n"
    "   40: SET_ALIAS  a1, r21\n"
    "   41: LABEL      L2\n"
    "   42: GET_ALIAS  r22, a4\n"
    "   43: ANDI       r23, r22, 268435455\n"
    "   44: GET_ALIAS  r24, a5\n"
    "   45: SLLI       r25, r24, 31\n"
    "   46: OR         r26, r23, r25\n"
    "   47: GET_ALIAS  r27, a6\n"
    "   48: SLLI       r28, r27, 30\n"
    "   49: OR         r29, r26, r28\n"
    "   50: GET_ALIAS  r30, a7\n"
    "   51: SLLI       r31, r30, 29\n"
    "   52: OR         r32, r29, r31\n"
    "   53: GET_ALIAS  r33, a8\n"
    "   54: SLLI       r34, r33, 28\n"
    "   55: OR         r35, r32, r34\n"
    "   56: SET_ALIAS  a4, r35\n"
    "   57: RETURN\n"
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
    "Alias 10: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,34] --> 1,2\n"
    "Block 1: 0 --> [35,37] --> 3\n"
    "Block 2: 0 --> [38,40] --> 3\n"
    "Block 3: 2,1 --> [41,57] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
