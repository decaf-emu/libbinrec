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
    0x10,0x42,0x10,0x2A,  // ps_add f2,f2,f2
    0x10,0x63,0x18,0x2A,  // ps_add f3,f3,f3
    0x10,0x22,0x20,0xFA,  // ps_madd f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FMADDS
                                    | BINREC_OPT_G_PPC_FAST_FMULS;
static const unsigned int common_opt = BINREC_OPT_NATIVE_IEEE_NAN;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FADD       r4, r3, r3\n"
    "    4: VFCVT      r5, r4\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: FADD       r7, r6, r6\n"
    "    7: VFCVT      r8, r7\n"
    "    8: VFCVT      r9, r5\n"
    "    9: VFCVT      r10, r8\n"
    "   10: GET_ALIAS  r11, a5\n"
    "   11: FMADD      r12, r9, r10, r11\n"
    "   12: VFCVT      r13, r12\n"
    "   13: VFCVT      r14, r13\n"
    "   14: SET_ALIAS  a2, r14\n"
    "   15: VFCVT      r15, r5\n"
    "   16: SET_ALIAS  a3, r15\n"
    "   17: VFCVT      r16, r8\n"
    "   18: SET_ALIAS  a4, r16\n"
    "   19: LOAD_IMM   r17, 12\n"
    "   20: SET_ALIAS  a1, r17\n"
    "   21: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,21] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
