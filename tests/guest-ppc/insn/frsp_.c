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
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
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
    "   32: LOAD_IMM   r23, 4\n"
    "   33: SET_ALIAS  a1, r23\n"
    "   34: GET_ALIAS  r24, a4\n"
    "   35: ANDI       r25, r24, -251658241\n"
    "   36: GET_ALIAS  r26, a5\n"
    "   37: SLLI       r27, r26, 27\n"
    "   38: OR         r28, r25, r27\n"
    "   39: GET_ALIAS  r29, a6\n"
    "   40: SLLI       r30, r29, 26\n"
    "   41: OR         r31, r28, r30\n"
    "   42: GET_ALIAS  r32, a7\n"
    "   43: SLLI       r33, r32, 25\n"
    "   44: OR         r34, r31, r33\n"
    "   45: GET_ALIAS  r35, a8\n"
    "   46: SLLI       r36, r35, 24\n"
    "   47: OR         r37, r34, r36\n"
    "   48: SET_ALIAS  a4, r37\n"
    "   49: RETURN\n"
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
    "Block 0: <none> --> [0,49] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
