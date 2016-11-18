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
    0xFC,0x20,0x10,0x19,  // frsp. f1,f2
    0x48,0x00,0x00,0x04,  // b 0x8
    /* This should not trigger a second warning. */
    0xFC,0x20,0x10,0x19,  // frsp. f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "[warning] Found floating-point instruction with Rc=1 (FC201019) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a4\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a6, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a7, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a8, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: FCVT       r9, r8\n"
    "   13: GET_ALIAS  r10, a9\n"
    "   14: ANDI       r11, r10, 33031936\n"
    "   15: SGTUI      r12, r11, 0\n"
    "   16: BFEXT      r13, r10, 25, 4\n"
    "   17: SLLI       r14, r12, 4\n"
    "   18: SRLI       r15, r10, 3\n"
    "   19: OR         r16, r13, r14\n"
    "   20: AND        r17, r16, r15\n"
    "   21: ANDI       r18, r17, 31\n"
    "   22: SGTUI      r19, r18, 0\n"
    "   23: BFEXT      r20, r10, 31, 1\n"
    "   24: BFEXT      r21, r10, 28, 1\n"
    "   25: SET_ALIAS  a5, r20\n"
    "   26: SET_ALIAS  a6, r19\n"
    "   27: SET_ALIAS  a7, r12\n"
    "   28: SET_ALIAS  a8, r21\n"
    "   29: FCVT       r22, r9\n"
    "   30: STORE      408(r1), r22\n"
    "   31: SET_ALIAS  a2, r22\n"
    "   32: GOTO       L1\n"
    "   33: LOAD_IMM   r23, 8\n"
    "   34: SET_ALIAS  a1, r23\n"
    "   35: LABEL      L1\n"
    "   36: GET_ALIAS  r24, a3\n"
    "   37: FCVT       r25, r24\n"
    "   38: GET_ALIAS  r26, a9\n"
    "   39: ANDI       r27, r26, 33031936\n"
    "   40: SGTUI      r28, r27, 0\n"
    "   41: BFEXT      r29, r26, 25, 4\n"
    "   42: SLLI       r30, r28, 4\n"
    "   43: SRLI       r31, r26, 3\n"
    "   44: OR         r32, r29, r30\n"
    "   45: AND        r33, r32, r31\n"
    "   46: ANDI       r34, r33, 31\n"
    "   47: SGTUI      r35, r34, 0\n"
    "   48: BFEXT      r36, r26, 31, 1\n"
    "   49: BFEXT      r37, r26, 28, 1\n"
    "   50: SET_ALIAS  a5, r36\n"
    "   51: SET_ALIAS  a6, r35\n"
    "   52: SET_ALIAS  a7, r28\n"
    "   53: SET_ALIAS  a8, r37\n"
    "   54: FCVT       r38, r25\n"
    "   55: STORE      408(r1), r38\n"
    "   56: SET_ALIAS  a2, r38\n"
    "   57: LOAD_IMM   r39, 12\n"
    "   58: SET_ALIAS  a1, r39\n"
    "   59: GET_ALIAS  r40, a4\n"
    "   60: ANDI       r41, r40, -251658241\n"
    "   61: GET_ALIAS  r42, a5\n"
    "   62: SLLI       r43, r42, 27\n"
    "   63: OR         r44, r41, r43\n"
    "   64: GET_ALIAS  r45, a6\n"
    "   65: SLLI       r46, r45, 26\n"
    "   66: OR         r47, r44, r46\n"
    "   67: GET_ALIAS  r48, a7\n"
    "   68: SLLI       r49, r48, 25\n"
    "   69: OR         r50, r47, r49\n"
    "   70: GET_ALIAS  r51, a8\n"
    "   71: SLLI       r52, r51, 24\n"
    "   72: OR         r53, r50, r52\n"
    "   73: SET_ALIAS  a4, r53\n"
    "   74: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,32] --> 2\n"
    "Block 1: <none> --> [33,34] --> 2\n"
    "Block 2: 1,0 --> [35,74] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
