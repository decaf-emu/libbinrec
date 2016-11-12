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
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a4, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: ANDI       r8, r7, -1611134977\n"
    "    9: SET_ALIAS  a3, r8\n"
    "   10: FGETSTATE  r9\n"
    "   11: ANDI       r10, r8, 3\n"
    "   12: GOTO_IF_Z  r10, L1\n"
    "   13: SLTUI      r11, r10, 2\n"
    "   14: GOTO_IF_NZ r11, L2\n"
    "   15: SEQI       r12, r10, 2\n"
    "   16: GOTO_IF_NZ r12, L3\n"
    "   17: FSETROUND  r13, r9, FLOOR\n"
    "   18: FSETSTATE  r13\n"
    "   19: GOTO       L4\n"
    "   20: LABEL      L3\n"
    "   21: FSETROUND  r14, r9, CEIL\n"
    "   22: FSETSTATE  r14\n"
    "   23: GOTO       L4\n"
    "   24: LABEL      L2\n"
    "   25: FSETROUND  r15, r9, TRUNC\n"
    "   26: FSETSTATE  r15\n"
    "   27: GOTO       L4\n"
    "   28: LABEL      L1\n"
    "   29: FSETROUND  r16, r9, NEAREST\n"
    "   30: FSETSTATE  r16\n"
    "   31: LABEL      L4\n"
    "   32: BFEXT      r17, r7, 12, 7\n"
    "   33: SET_ALIAS  a4, r17\n"
    "   34: LOAD_IMM   r18, 4\n"
    "   35: SET_ALIAS  a1, r18\n"
    "   36: GET_ALIAS  r19, a3\n"
    "   37: GET_ALIAS  r20, a4\n"
    "   38: ANDI       r21, r19, -1611134977\n"
    "   39: SLLI       r22, r20, 12\n"
    "   40: OR         r23, r21, r22\n"
    "   41: SET_ALIAS  a3, r23\n"
    "   42: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: int32 @ 944(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,6\n"
    "Block 1: 0 --> [13,14] --> 2,5\n"
    "Block 2: 1 --> [15,16] --> 3,4\n"
    "Block 3: 2 --> [17,19] --> 7\n"
    "Block 4: 2 --> [20,23] --> 7\n"
    "Block 5: 1 --> [24,27] --> 7\n"
    "Block 6: 0 --> [28,30] --> 7\n"
    "Block 7: 6,3,4,5 --> [31,42] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
