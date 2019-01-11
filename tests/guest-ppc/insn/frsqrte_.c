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
    0xFC,0x20,0x10,0x35,  // frsqrte. f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_NATIVE_RECIPROCAL;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (FC201035) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FSQRT      r4, r3\n"
    "    4: LOAD_IMM   r5, 1.0\n"
    "    5: FDIV       r6, r5, r4\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: ANDI       r8, r7, 33031936\n"
    "    8: SGTUI      r9, r8, 0\n"
    "    9: BFEXT      r10, r7, 25, 4\n"
    "   10: SLLI       r11, r9, 4\n"
    "   11: SRLI       r12, r7, 3\n"
    "   12: OR         r13, r10, r11\n"
    "   13: AND        r14, r13, r12\n"
    "   14: ANDI       r15, r14, 31\n"
    "   15: SGTUI      r16, r15, 0\n"
    "   16: BFEXT      r17, r7, 31, 1\n"
    "   17: BFEXT      r18, r7, 28, 1\n"
    "   18: GET_ALIAS  r19, a4\n"
    "   19: SLLI       r20, r17, 3\n"
    "   20: SLLI       r21, r16, 2\n"
    "   21: SLLI       r22, r9, 1\n"
    "   22: OR         r23, r20, r21\n"
    "   23: OR         r24, r22, r18\n"
    "   24: OR         r25, r23, r24\n"
    "   25: BFINS      r26, r19, r25, 24, 4\n"
    "   26: SET_ALIAS  a4, r26\n"
    "   27: SET_ALIAS  a2, r6\n"
    "   28: LOAD_IMM   r27, 4\n"
    "   29: SET_ALIAS  a1, r27\n"
    "   30: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,30] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
