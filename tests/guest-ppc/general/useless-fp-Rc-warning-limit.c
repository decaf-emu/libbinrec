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
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FCVT       r4, r3\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: ANDI       r6, r5, 33031936\n"
    "    6: SGTUI      r7, r6, 0\n"
    "    7: BFEXT      r8, r5, 25, 4\n"
    "    8: SLLI       r9, r7, 4\n"
    "    9: SRLI       r10, r5, 3\n"
    "   10: OR         r11, r8, r9\n"
    "   11: AND        r12, r11, r10\n"
    "   12: ANDI       r13, r12, 31\n"
    "   13: SGTUI      r14, r13, 0\n"
    "   14: BFEXT      r15, r5, 31, 1\n"
    "   15: BFEXT      r16, r5, 28, 1\n"
    "   16: GET_ALIAS  r17, a4\n"
    "   17: SLLI       r18, r15, 3\n"
    "   18: SLLI       r19, r14, 2\n"
    "   19: SLLI       r20, r7, 1\n"
    "   20: OR         r21, r18, r19\n"
    "   21: OR         r22, r20, r16\n"
    "   22: OR         r23, r21, r22\n"
    "   23: BFINS      r24, r17, r23, 24, 4\n"
    "   24: SET_ALIAS  a4, r24\n"
    "   25: FCVT       r25, r4\n"
    "   26: STORE      408(r1), r25\n"
    "   27: SET_ALIAS  a2, r25\n"
    "   28: GOTO       L1\n"
    "   29: LOAD_IMM   r26, 8\n"
    "   30: SET_ALIAS  a1, r26\n"
    "   31: LABEL      L1\n"
    "   32: GET_ALIAS  r27, a3\n"
    "   33: FCVT       r28, r27\n"
    "   34: GET_ALIAS  r29, a5\n"
    "   35: ANDI       r30, r29, 33031936\n"
    "   36: SGTUI      r31, r30, 0\n"
    "   37: BFEXT      r32, r29, 25, 4\n"
    "   38: SLLI       r33, r31, 4\n"
    "   39: SRLI       r34, r29, 3\n"
    "   40: OR         r35, r32, r33\n"
    "   41: AND        r36, r35, r34\n"
    "   42: ANDI       r37, r36, 31\n"
    "   43: SGTUI      r38, r37, 0\n"
    "   44: BFEXT      r39, r29, 31, 1\n"
    "   45: BFEXT      r40, r29, 28, 1\n"
    "   46: GET_ALIAS  r41, a4\n"
    "   47: SLLI       r42, r39, 3\n"
    "   48: SLLI       r43, r38, 2\n"
    "   49: SLLI       r44, r31, 1\n"
    "   50: OR         r45, r42, r43\n"
    "   51: OR         r46, r44, r40\n"
    "   52: OR         r47, r45, r46\n"
    "   53: BFINS      r48, r41, r47, 24, 4\n"
    "   54: SET_ALIAS  a4, r48\n"
    "   55: FCVT       r49, r28\n"
    "   56: STORE      408(r1), r49\n"
    "   57: SET_ALIAS  a2, r49\n"
    "   58: LOAD_IMM   r50, 12\n"
    "   59: SET_ALIAS  a1, r50\n"
    "   60: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,28] --> 2\n"
    "Block 1: <none> --> [29,30] --> 2\n"
    "Block 2: 1,0 --> [31,60] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
