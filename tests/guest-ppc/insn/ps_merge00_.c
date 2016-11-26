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

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FCAST      r4, r3\n"
    "    4: GET_ALIAS  r5, a4\n"
    "    5: FGETSTATE  r6\n"
    "    6: FSETROUND  r7, r6, TRUNC\n"
    "    7: FSETSTATE  r7\n"
    "    8: FCAST      r8, r5\n"
    "    9: FSETSTATE  r6\n"
    "   10: VBUILD2    r9, r4, r8\n"
    "   11: GET_ALIAS  r10, a6\n"
    "   12: ANDI       r11, r10, 33031936\n"
    "   13: SGTUI      r12, r11, 0\n"
    "   14: BFEXT      r13, r10, 25, 4\n"
    "   15: SLLI       r14, r12, 4\n"
    "   16: SRLI       r15, r10, 3\n"
    "   17: OR         r16, r13, r14\n"
    "   18: AND        r17, r16, r15\n"
    "   19: ANDI       r18, r17, 31\n"
    "   20: SGTUI      r19, r18, 0\n"
    "   21: BFEXT      r20, r10, 31, 1\n"
    "   22: BFEXT      r21, r10, 28, 1\n"
    "   23: GET_ALIAS  r22, a5\n"
    "   24: SLLI       r23, r20, 3\n"
    "   25: SLLI       r24, r19, 2\n"
    "   26: SLLI       r25, r12, 1\n"
    "   27: OR         r26, r23, r24\n"
    "   28: OR         r27, r25, r21\n"
    "   29: OR         r28, r26, r27\n"
    "   30: BFINS      r29, r22, r28, 24, 4\n"
    "   31: SET_ALIAS  a5, r29\n"
    "   32: VFCAST     r30, r9\n"
    "   33: SET_ALIAS  a2, r30\n"
    "   34: LOAD_IMM   r31, 4\n"
    "   35: SET_ALIAS  a1, r31\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: float64 @ 432(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
