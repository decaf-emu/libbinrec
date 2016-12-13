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
    0x10,0x22,0x1C,0x21,  // ps_merge00. f1,f2,f3
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FGETSTATE  r4\n"
    "    4: FCVT       r5, r3\n"
    "    5: FSETSTATE  r4\n"
    "    6: GET_ALIAS  r6, a4\n"
    "    7: FGETSTATE  r7\n"
    "    8: FSETROUND  r8, r7, TRUNC\n"
    "    9: FSETSTATE  r8\n"
    "   10: FCVT       r9, r6\n"
    "   11: FSETSTATE  r7\n"
    "   12: VBUILD2    r10, r5, r9\n"
    "   13: GET_ALIAS  r11, a6\n"
    "   14: ANDI       r12, r11, 33031936\n"
    "   15: SGTUI      r13, r12, 0\n"
    "   16: BFEXT      r14, r11, 25, 4\n"
    "   17: SLLI       r15, r13, 4\n"
    "   18: SRLI       r16, r11, 3\n"
    "   19: OR         r17, r14, r15\n"
    "   20: AND        r18, r17, r16\n"
    "   21: ANDI       r19, r18, 31\n"
    "   22: SGTUI      r20, r19, 0\n"
    "   23: BFEXT      r21, r11, 31, 1\n"
    "   24: BFEXT      r22, r11, 28, 1\n"
    "   25: GET_ALIAS  r23, a5\n"
    "   26: SLLI       r24, r21, 3\n"
    "   27: SLLI       r25, r20, 2\n"
    "   28: SLLI       r26, r13, 1\n"
    "   29: OR         r27, r24, r25\n"
    "   30: OR         r28, r26, r22\n"
    "   31: OR         r29, r27, r28\n"
    "   32: BFINS      r30, r23, r29, 24, 4\n"
    "   33: SET_ALIAS  a5, r30\n"
    "   34: VFCVT      r31, r10\n"
    "   35: SET_ALIAS  a2, r31\n"
    "   36: LOAD_IMM   r32, 4\n"
    "   37: SET_ALIAS  a1, r32\n"
    "   38: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,38] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
