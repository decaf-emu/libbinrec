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
    0xC0,0x43,0x00,0x00,  // lfs f2,0(r3)
    0x10,0x22,0x20,0xEE,  // ps_sel f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, 0(r5)\n"
    "    6: FGETSTATE  r7\n"
    "    7: VBROADCAST r8, r6\n"
    "    8: GET_ALIAS  r9, a5\n"
    "    9: GET_ALIAS  r10, a6\n"
    "   10: LOAD_IMM   r11, 0.0f\n"
    "   11: VEXTRACT   r12, r8, 0\n"
    "   12: VEXTRACT   r13, r9, 0\n"
    "   13: VEXTRACT   r14, r10, 0\n"
    "   14: FCMP       r15, r12, r11, GE\n"
    "   15: SELECT     r16, r13, r14, r15\n"
    "   16: VEXTRACT   r17, r8, 1\n"
    "   17: VEXTRACT   r18, r9, 1\n"
    "   18: VEXTRACT   r19, r10, 1\n"
    "   19: FCMP       r20, r17, r11, GE\n"
    "   20: SELECT     r21, r18, r19, r20\n"
    "   21: VBUILD2    r22, r16, r21\n"
    "   22: FSETSTATE  r7\n"
    "   23: SET_ALIAS  a3, r22\n"
    "   24: FCVT       r23, r6\n"
    "   25: VBROADCAST r24, r23\n"
    "   26: SET_ALIAS  a4, r24\n"
    "   27: LOAD_IMM   r25, 8\n"
    "   28: SET_ALIAS  a1, r25\n"
    "   29: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2] @ 416(r1)\n"
    "Alias 5: float64[2] @ 432(r1)\n"
    "Alias 6: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,29] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
