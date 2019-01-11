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
    0x7F,0x80,0x04,0x00,  // mcrxr cr7
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BFEXT      r4, r3, 3, 1\n"
    "    4: SET_ALIAS  a3, r4\n"
    "    5: BFEXT      r5, r3, 2, 1\n"
    "    6: SET_ALIAS  a4, r5\n"
    "    7: BFEXT      r6, r3, 1, 1\n"
    "    8: SET_ALIAS  a5, r6\n"
    "    9: BFEXT      r7, r3, 0, 1\n"
    "   10: SET_ALIAS  a6, r7\n"
    "   11: GET_ALIAS  r8, a7\n"
    "   12: BFEXT      r9, r8, 31, 1\n"
    "   13: BFEXT      r10, r8, 30, 1\n"
    "   14: BFEXT      r11, r8, 29, 1\n"
    "   15: BFEXT      r12, r8, 28, 1\n"
    "   16: SET_ALIAS  a3, r9\n"
    "   17: SET_ALIAS  a4, r10\n"
    "   18: SET_ALIAS  a5, r11\n"
    "   19: SET_ALIAS  a6, r12\n"
    "   20: ANDI       r13, r8, 268435455\n"
    "   21: LOAD_IMM   r14, 0\n"
    "   22: SET_ALIAS  a7, r13\n"
    "   23: LOAD_IMM   r15, 4\n"
    "   24: SET_ALIAS  a1, r15\n"
    "   25: GET_ALIAS  r16, a2\n"
    "   26: ANDI       r17, r16, -16\n"
    "   27: GET_ALIAS  r18, a3\n"
    "   28: SLLI       r19, r18, 3\n"
    "   29: OR         r20, r17, r19\n"
    "   30: GET_ALIAS  r21, a4\n"
    "   31: SLLI       r22, r21, 2\n"
    "   32: OR         r23, r20, r22\n"
    "   33: GET_ALIAS  r24, a5\n"
    "   34: SLLI       r25, r24, 1\n"
    "   35: OR         r26, r23, r25\n"
    "   36: GET_ALIAS  r27, a6\n"
    "   37: OR         r28, r26, r27\n"
    "   38: SET_ALIAS  a2, r28\n"
    "   39: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 928(r1)\n"
    "Alias 3: int32, no bound storage\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,39] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
