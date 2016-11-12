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
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a5\n"
    "    3: BFEXT      r4, r3, 27, 1\n"
    "    4: SET_ALIAS  a6, r4\n"
    "    5: BFEXT      r5, r3, 26, 1\n"
    "    6: SET_ALIAS  a7, r5\n"
    "    7: BFEXT      r6, r3, 25, 1\n"
    "    8: SET_ALIAS  a8, r6\n"
    "    9: BFEXT      r7, r3, 24, 1\n"
    "   10: SET_ALIAS  a9, r7\n"
    "   11: GET_ALIAS  r8, a3\n"
    "   12: GET_ALIAS  r9, a4\n"
    "   13: FADD       r10, r8, r9\n"
    "   14: GET_ALIAS  r11, a10\n"
    "   15: ANDI       r12, r11, 33031936\n"
    "   16: SGTUI      r13, r12, 0\n"
    "   17: BFEXT      r14, r11, 25, 4\n"
    "   18: SLLI       r15, r13, 4\n"
    "   19: SRLI       r16, r11, 3\n"
    "   20: OR         r17, r14, r15\n"
    "   21: AND        r18, r17, r16\n"
    "   22: ANDI       r19, r18, 31\n"
    "   23: SGTUI      r20, r19, 0\n"
    "   24: BFEXT      r21, r11, 31, 1\n"
    "   25: BFEXT      r22, r11, 28, 1\n"
    "   26: SET_ALIAS  a6, r21\n"
    "   27: SET_ALIAS  a7, r20\n"
    "   28: SET_ALIAS  a8, r13\n"
    "   29: SET_ALIAS  a9, r22\n"
    "   30: SET_ALIAS  a2, r10\n"
    "   31: LOAD_IMM   r23, 4\n"
    "   32: SET_ALIAS  a1, r23\n"
    "   33: GET_ALIAS  r24, a5\n"
    "   34: ANDI       r25, r24, -251658241\n"
    "   35: GET_ALIAS  r26, a6\n"
    "   36: SLLI       r27, r26, 27\n"
    "   37: OR         r28, r25, r27\n"
    "   38: GET_ALIAS  r29, a7\n"
    "   39: SLLI       r30, r29, 26\n"
    "   40: OR         r31, r28, r30\n"
    "   41: GET_ALIAS  r32, a8\n"
    "   42: SLLI       r33, r32, 25\n"
    "   43: OR         r34, r31, r33\n"
    "   44: GET_ALIAS  r35, a9\n"
    "   45: SLLI       r36, r35, 24\n"
    "   46: OR         r37, r34, r36\n"
    "   47: SET_ALIAS  a5, r37\n"
    "   48: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32, no bound storage\n"
    "Alias 10: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,48] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
