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
    0xFD,0xFE,0x0D,0x8E,  // mtfs f1 (mtfsf 255,f1)
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: ANDI       r8, r7, -1611134977\n"
    "    9: SET_ALIAS  a3, r8\n"
    "   10: ANDI       r9, r8, 3\n"
    "   11: GOTO_IF_Z  r9, L1\n"
    "   12: SLTUI      r10, r9, 2\n"
    "   13: GOTO_IF_NZ r10, L2\n"
    "   14: SEQI       r11, r9, 2\n"
    "   15: GOTO_IF_NZ r11, L3\n"
    "   16: FSETROUND  FLOOR\n"
    "   17: GOTO       L4\n"
    "   18: LABEL      L3\n"
    "   19: FSETROUND  CEIL\n"
    "   20: GOTO       L4\n"
    "   21: LABEL      L2\n"
    "   22: FSETROUND  TRUNC\n"
    "   23: GOTO       L4\n"
    "   24: LABEL      L1\n"
    "   25: FSETROUND  NEAREST\n"
    "   26: LABEL      L4\n"
    "   27: BFEXT      r12, r7, 12, 7\n"
    "   28: SET_ALIAS  a4, r12\n"
    "   29: LOAD_IMM   r13, 4\n"
    "   30: SET_ALIAS  a1, r13\n"
    "   31: GET_ALIAS  r14, a3\n"
    "   32: GET_ALIAS  r15, a4\n"
    "   33: ANDI       r16, r14, -1611134977\n"
    "   34: SLLI       r17, r15, 12\n"
    "   35: OR         r18, r16, r17\n"
    "   36: SET_ALIAS  a3, r18\n"
    "   37: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,11] --> 1,6\n"
    "Block 1: 0 --> [12,13] --> 2,5\n"
    "Block 2: 1 --> [14,15] --> 3,4\n"
    "Block 3: 2 --> [16,17] --> 7\n"
    "Block 4: 2 --> [18,20] --> 7\n"
    "Block 5: 1 --> [21,23] --> 7\n"
    "Block 6: 0 --> [24,25] --> 7\n"
    "Block 7: 6,3,4,5 --> [26,37] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
