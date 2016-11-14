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
    0x4F,0xDE,0xF2,0x42,  // crset 30
    0x4F,0xFF,0xF9,0x82,  // crclr 31
    0x41,0x9E,0x00,0x08,  // beq cr7,0x10
    0x7F,0xE0,0x00,0x08,  // trap
    0x4F,0xDE,0xF1,0x82,  // crclr 30
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_TRIM_CR_STORES;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x13\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 3\n"
        "[info] r3 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: LOAD_IMM   r3, 1\n"
    "    3: NOP\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a5, r4\n"
    "    6: GOTO_IF_NZ r3, L1\n"
    "    7: SET_ALIAS  a4, r3\n"
    "    8: LOAD_IMM   r5, 12\n"
    "    9: SET_ALIAS  a1, r5\n"
    "   10: GET_ALIAS  r6, a2\n"
    "   11: GET_ALIAS  r7, a3\n"
    "   12: ANDI       r8, r7, -4\n"
    "   13: GET_ALIAS  r9, a4\n"
    "   14: SLLI       r10, r9, 1\n"
    "   15: OR         r11, r8, r10\n"
    "   16: GET_ALIAS  r12, a5\n"
    "   17: OR         r13, r11, r12\n"
    "   18: SET_ALIAS  a3, r13\n"
    "   19: LOAD_IMM   r14, 12\n"
    "   20: SET_ALIAS  a1, r14\n"
    "   21: LOAD       r15, 984(r1)\n"
    "   22: CALL       @r15, r1\n"
    "   23: RETURN\n"
    "   24: LOAD_IMM   r16, 16\n"
    "   25: SET_ALIAS  a1, r16\n"
    "   26: LABEL      L1\n"
    "   27: LOAD_IMM   r17, 0\n"
    "   28: SET_ALIAS  a4, r17\n"
    "   29: LOAD_IMM   r18, 20\n"
    "   30: SET_ALIAS  a1, r18\n"
    "   31: GET_ALIAS  r19, a3\n"
    "   32: ANDI       r20, r19, -4\n"
    "   33: GET_ALIAS  r21, a4\n"
    "   34: SLLI       r22, r21, 1\n"
    "   35: OR         r23, r20, r22\n"
    "   36: GET_ALIAS  r24, a5\n"
    "   37: OR         r25, r23, r24\n"
    "   38: SET_ALIAS  a3, r25\n"
    "   39: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 1,3\n"
    "Block 1: 0 --> [7,23] --> <none>\n"
    "Block 2: <none> --> [24,25] --> 3\n"
    "Block 3: 2,0 --> [26,39] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
