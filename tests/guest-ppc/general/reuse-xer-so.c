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
    0x2F,0x03,0x00,0x01,  // cmpi cr6,r3,1
    0x2F,0x83,0x00,0x02,  // cmpi cr7,r3,2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 16\n"
        "[info] r1 death rolled back to 8\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: SLTSI      r4, r3, 1\n"
    "    4: SGTSI      r5, r3, 1\n"
    "    5: SEQI       r6, r3, 1\n"
    "    6: GET_ALIAS  r7, a4\n"
    "    7: BFEXT      r8, r7, 31, 1\n"
    "    8: GET_ALIAS  r9, a3\n"
    "    9: SLLI       r10, r4, 3\n"
    "   10: SLLI       r11, r5, 2\n"
    "   11: SLLI       r12, r6, 1\n"
    "   12: OR         r13, r10, r11\n"
    "   13: OR         r14, r12, r8\n"
    "   14: OR         r15, r13, r14\n"
    "   15: BFINS      r16, r9, r15, 4, 4\n"
    "   16: NOP\n"
    "   17: SLTSI      r17, r3, 2\n"
    "   18: SGTSI      r18, r3, 2\n"
    "   19: SEQI       r19, r3, 2\n"
    "   20: SLLI       r20, r17, 3\n"
    "   21: SLLI       r21, r18, 2\n"
    "   22: SLLI       r22, r19, 1\n"
    "   23: OR         r23, r20, r21\n"
    "   24: OR         r24, r22, r8\n"
    "   25: OR         r25, r23, r24\n"
    "   26: BFINS      r26, r16, r25, 0, 4\n"
    "   27: SET_ALIAS  a3, r26\n"
    "   28: LOAD_IMM   r27, 8\n"
    "   29: SET_ALIAS  a1, r27\n"
    "   30: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 928(r1)\n"
    "Alias 4: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
