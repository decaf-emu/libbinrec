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
    0x10,0x22,0x19,0x15,  // ps_sum0. f1,f2,f4,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "[warning] Found floating-point instruction with Rc=1 (10221915) at 0x0 but NO_FPSCR_STATE optimization is enabled; exceptions will not be detected (this warning is reported only once per translation unit)\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: VEXTRACT   r4, r3, 0\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: VEXTRACT   r6, r5, 1\n"
    "    6: GET_ALIAS  r7, a5\n"
    "    7: VEXTRACT   r8, r7, 1\n"
    "    8: FCVT       r9, r8\n"
    "    9: FADD       r10, r4, r6\n"
    "   10: FCVT       r11, r10\n"
    "   11: VBUILD2    r12, r11, r9\n"
    "   12: GET_ALIAS  r13, a7\n"
    "   13: ANDI       r14, r13, 33031936\n"
    "   14: SGTUI      r15, r14, 0\n"
    "   15: BFEXT      r16, r13, 25, 4\n"
    "   16: SLLI       r17, r15, 4\n"
    "   17: SRLI       r18, r13, 3\n"
    "   18: OR         r19, r16, r17\n"
    "   19: AND        r20, r19, r18\n"
    "   20: ANDI       r21, r20, 31\n"
    "   21: SGTUI      r22, r21, 0\n"
    "   22: BFEXT      r23, r13, 31, 1\n"
    "   23: BFEXT      r24, r13, 28, 1\n"
    "   24: GET_ALIAS  r25, a6\n"
    "   25: SLLI       r26, r23, 3\n"
    "   26: SLLI       r27, r22, 2\n"
    "   27: SLLI       r28, r15, 1\n"
    "   28: OR         r29, r26, r27\n"
    "   29: OR         r30, r28, r24\n"
    "   30: OR         r31, r29, r30\n"
    "   31: BFINS      r32, r25, r31, 24, 4\n"
    "   32: SET_ALIAS  a6, r32\n"
    "   33: VFCVT      r33, r12\n"
    "   34: SET_ALIAS  a2, r33\n"
    "   35: LOAD_IMM   r34, 4\n"
    "   36: SET_ALIAS  a1, r34\n"
    "   37: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "Alias 6: int32 @ 928(r1)\n"
    "Alias 7: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,37] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
