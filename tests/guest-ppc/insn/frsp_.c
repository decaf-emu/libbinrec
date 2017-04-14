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
    "   28: LOAD_IMM   r26, 4\n"
    "   29: SET_ALIAS  a1, r26\n"
    "   30: RETURN\n"
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
