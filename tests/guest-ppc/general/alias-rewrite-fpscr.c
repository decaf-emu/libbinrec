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
    0xFD,0xFE,0x0D,0x8E,  // mtfsf 255,f1
    0xFD,0xFE,0x15,0x8E,  // mtfsf 255,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_USE_SPLIT_FIELDS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 9\n"
        "[info] Killing instruction 33\n"
        "[info] r17 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: BITCAST    r6, r5\n"
    "    7: ZCAST      r7, r6\n"
    "    8: ANDI       r8, r7, -1611134977\n"
    "    9: NOP\n"
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
    "   33: NOP\n"
    "   34: GET_ALIAS  r18, a3\n"
    "   35: BITCAST    r19, r18\n"
    "   36: ZCAST      r20, r19\n"
    "   37: ANDI       r21, r20, -1611134977\n"
    "   38: SET_ALIAS  a4, r21\n"
    "   39: FGETSTATE  r22\n"
    "   40: ANDI       r23, r21, 3\n"
    "   41: GOTO_IF_Z  r23, L5\n"
    "   42: SLTUI      r24, r23, 2\n"
    "   43: GOTO_IF_NZ r24, L6\n"
    "   44: SEQI       r25, r23, 2\n"
    "   45: GOTO_IF_NZ r25, L7\n"
    "   46: FSETROUND  r26, r22, FLOOR\n"
    "   47: FSETSTATE  r26\n"
    "   48: GOTO       L8\n"
    "   49: LABEL      L7\n"
    "   50: FSETROUND  r27, r22, CEIL\n"
    "   51: FSETSTATE  r27\n"
    "   52: GOTO       L8\n"
    "   53: LABEL      L6\n"
    "   54: FSETROUND  r28, r22, TRUNC\n"
    "   55: FSETSTATE  r28\n"
    "   56: GOTO       L8\n"
    "   57: LABEL      L5\n"
    "   58: FSETROUND  r29, r22, NEAREST\n"
    "   59: FSETSTATE  r29\n"
    "   60: LABEL      L8\n"
    "   61: BFEXT      r30, r20, 12, 7\n"
    "   62: SET_ALIAS  a5, r30\n"
    "   63: LOAD_IMM   r31, 8\n"
    "   64: SET_ALIAS  a1, r31\n"
    "   65: GET_ALIAS  r32, a4\n"
    "   66: GET_ALIAS  r33, a5\n"
    "   67: ANDI       r34, r32, -1611134977\n"
    "   68: SLLI       r35, r33, 12\n"
    "   69: OR         r36, r34, r35\n"
    "   70: SET_ALIAS  a4, r36\n"
    "   71: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 944(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,6\n"
    "Block 1: 0 --> [13,14] --> 2,5\n"
    "Block 2: 1 --> [15,16] --> 3,4\n"
    "Block 3: 2 --> [17,19] --> 7\n"
    "Block 4: 2 --> [20,23] --> 7\n"
    "Block 5: 1 --> [24,27] --> 7\n"
    "Block 6: 0 --> [28,30] --> 7\n"
    "Block 7: 6,3,4,5 --> [31,41] --> 8,13\n"
    "Block 8: 7 --> [42,43] --> 9,12\n"
    "Block 9: 8 --> [44,45] --> 10,11\n"
    "Block 10: 9 --> [46,48] --> 14\n"
    "Block 11: 9 --> [49,52] --> 14\n"
    "Block 12: 8 --> [53,56] --> 14\n"
    "Block 13: 7 --> [57,59] --> 14\n"
    "Block 14: 13,10,11,12 --> [60,71] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
