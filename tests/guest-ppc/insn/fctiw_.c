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
    0xFC,0x20,0x10,0x1D,  // fctiw. f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FCTIW;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (FC20101D) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FROUNDI    r4, r3\n"
    "    4: LOAD_IMM   r5, 2147483647.0\n"
    "    5: FCMP       r6, r3, r5, GE\n"
    "    6: LOAD_IMM   r7, 0x7FFFFFFF\n"
    "    7: SELECT     r8, r7, r4, r6\n"
    "    8: ZCAST      r9, r8\n"
    "    9: BITCAST    r10, r9\n"
    "   10: GET_ALIAS  r11, a5\n"
    "   11: ANDI       r12, r11, 33031936\n"
    "   12: SGTUI      r13, r12, 0\n"
    "   13: BFEXT      r14, r11, 25, 4\n"
    "   14: SLLI       r15, r13, 4\n"
    "   15: SRLI       r16, r11, 3\n"
    "   16: OR         r17, r14, r15\n"
    "   17: AND        r18, r17, r16\n"
    "   18: ANDI       r19, r18, 31\n"
    "   19: SGTUI      r20, r19, 0\n"
    "   20: BFEXT      r21, r11, 31, 1\n"
    "   21: BFEXT      r22, r11, 28, 1\n"
    "   22: GET_ALIAS  r23, a4\n"
    "   23: SLLI       r24, r21, 3\n"
    "   24: SLLI       r25, r20, 2\n"
    "   25: SLLI       r26, r13, 1\n"
    "   26: OR         r27, r24, r25\n"
    "   27: OR         r28, r26, r22\n"
    "   28: OR         r29, r27, r28\n"
    "   29: BFINS      r30, r23, r29, 24, 4\n"
    "   30: SET_ALIAS  a4, r30\n"
    "   31: SET_ALIAS  a2, r10\n"
    "   32: LOAD_IMM   r31, 4\n"
    "   33: SET_ALIAS  a1, r31\n"
    "   34: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
