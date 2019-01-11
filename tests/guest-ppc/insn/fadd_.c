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
    0xFC,0x22,0x18,0x2B,  // fadd. f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (FC22182B) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: FADD       r5, r3, r4\n"
    "    5: GET_ALIAS  r6, a6\n"
    "    6: ANDI       r7, r6, 33031936\n"
    "    7: SGTUI      r8, r7, 0\n"
    "    8: BFEXT      r9, r6, 25, 4\n"
    "    9: SLLI       r10, r8, 4\n"
    "   10: SRLI       r11, r6, 3\n"
    "   11: OR         r12, r9, r10\n"
    "   12: AND        r13, r12, r11\n"
    "   13: ANDI       r14, r13, 31\n"
    "   14: SGTUI      r15, r14, 0\n"
    "   15: BFEXT      r16, r6, 31, 1\n"
    "   16: BFEXT      r17, r6, 28, 1\n"
    "   17: GET_ALIAS  r18, a5\n"
    "   18: SLLI       r19, r16, 3\n"
    "   19: SLLI       r20, r15, 2\n"
    "   20: SLLI       r21, r8, 1\n"
    "   21: OR         r22, r19, r20\n"
    "   22: OR         r23, r21, r17\n"
    "   23: OR         r24, r22, r23\n"
    "   24: BFINS      r25, r18, r24, 24, 4\n"
    "   25: SET_ALIAS  a5, r25\n"
    "   26: SET_ALIAS  a2, r5\n"
    "   27: LOAD_IMM   r26, 4\n"
    "   28: SET_ALIAS  a1, r26\n"
    "   29: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
