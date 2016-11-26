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
    0x10,0x22,0x20,0xFB,  // ps_madd. f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FMULS;
static const unsigned int common_opt = BINREC_OPT_NATIVE_IEEE_NAN;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (102220FB) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: FMADD      r6, r3, r4, r5\n"
    "    6: VFCVT      r7, r6\n"
    "    7: GET_ALIAS  r8, a7\n"
    "    8: ANDI       r9, r8, 33031936\n"
    "    9: SGTUI      r10, r9, 0\n"
    "   10: BFEXT      r11, r8, 25, 4\n"
    "   11: SLLI       r12, r10, 4\n"
    "   12: SRLI       r13, r8, 3\n"
    "   13: OR         r14, r11, r12\n"
    "   14: AND        r15, r14, r13\n"
    "   15: ANDI       r16, r15, 31\n"
    "   16: SGTUI      r17, r16, 0\n"
    "   17: BFEXT      r18, r8, 31, 1\n"
    "   18: BFEXT      r19, r8, 28, 1\n"
    "   19: GET_ALIAS  r20, a6\n"
    "   20: SLLI       r21, r18, 3\n"
    "   21: SLLI       r22, r17, 2\n"
    "   22: SLLI       r23, r10, 1\n"
    "   23: OR         r24, r21, r22\n"
    "   24: OR         r25, r23, r19\n"
    "   25: OR         r26, r24, r25\n"
    "   26: BFINS      r27, r20, r26, 24, 4\n"
    "   27: SET_ALIAS  a6, r27\n"
    "   28: VFCVT      r28, r7\n"
    "   29: SET_ALIAS  a2, r28\n"
    "   30: LOAD_IMM   r29, 4\n"
    "   31: SET_ALIAS  a1, r29\n"
    "   32: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,32] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
