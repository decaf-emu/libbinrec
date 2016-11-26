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
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: BITCAST    r4, r3\n"
    "    4: ZCAST      r5, r4\n"
    "    5: ANDI       r6, r5, -1610614785\n"
    "    6: SET_ALIAS  a3, r6\n"
    "    7: FGETSTATE  r7\n"
    "    8: ANDI       r8, r6, 3\n"
    "    9: GOTO_IF_Z  r8, L1\n"
    "   10: SLTUI      r9, r8, 2\n"
    "   11: GOTO_IF_NZ r9, L2\n"
    "   12: SEQI       r10, r8, 2\n"
    "   13: GOTO_IF_NZ r10, L3\n"
    "   14: FSETROUND  r11, r7, FLOOR\n"
    "   15: FSETSTATE  r11\n"
    "   16: GOTO       L4\n"
    "   17: LABEL      L3\n"
    "   18: FSETROUND  r12, r7, CEIL\n"
    "   19: FSETSTATE  r12\n"
    "   20: GOTO       L4\n"
    "   21: LABEL      L2\n"
    "   22: FSETROUND  r13, r7, TRUNC\n"
    "   23: FSETSTATE  r13\n"
    "   24: GOTO       L4\n"
    "   25: LABEL      L1\n"
    "   26: FSETROUND  r14, r7, NEAREST\n"
    "   27: FSETSTATE  r14\n"
    "   28: LABEL      L4\n"
    "   29: LOAD_IMM   r15, 4\n"
    "   30: SET_ALIAS  a1, r15\n"
    "   31: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,9] --> 1,6\n"
    "Block 1: 0 --> [10,11] --> 2,5\n"
    "Block 2: 1 --> [12,13] --> 3,4\n"
    "Block 3: 2 --> [14,16] --> 7\n"
    "Block 4: 2 --> [17,20] --> 7\n"
    "Block 5: 1 --> [21,24] --> 7\n"
    "Block 6: 0 --> [25,27] --> 7\n"
    "Block 7: 6,3,4,5 --> [28,31] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
