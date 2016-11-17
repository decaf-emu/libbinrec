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
    0x10,0x22,0x20,0xFE,  // ps_nmadd f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FMULS;
static const unsigned int common_opt = BINREC_OPT_NATIVE_IEEE_NAN;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: VEXTRACT   r6, r3, 0\n"
    "    6: VEXTRACT   r7, r3, 1\n"
    "    7: VEXTRACT   r8, r4, 0\n"
    "    8: VEXTRACT   r9, r4, 1\n"
    "    9: VEXTRACT   r10, r5, 0\n"
    "   10: VEXTRACT   r11, r5, 1\n"
    "   11: FMADD      r12, r6, r8, r10\n"
    "   12: LOAD_IMM   r13, 0.0\n"
    "   13: FCMP       r14, r12, r13, UN\n"
    "   14: FNEG       r15, r12\n"
    "   15: SELECT     r16, r12, r15, r14\n"
    "   16: FCVT       r17, r16\n"
    "   17: FMADD      r18, r7, r9, r11\n"
    "   18: LOAD_IMM   r19, 0.0\n"
    "   19: FCMP       r20, r18, r19, UN\n"
    "   20: FNEG       r21, r18\n"
    "   21: SELECT     r22, r18, r21, r20\n"
    "   22: FCVT       r23, r22\n"
    "   23: VBUILD2    r24, r17, r23\n"
    "   24: VFCVT      r25, r24\n"
    "   25: SET_ALIAS  a2, r25\n"
    "   26: LOAD_IMM   r26, 4\n"
    "   27: SET_ALIAS  a1, r26\n"
    "   28: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,28] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
