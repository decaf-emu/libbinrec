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
    0xFC,0x20,0x10,0x1C,  // fctiw f1,f2
    0x10,0x20,0x08,0x50,  // ps_neg f1,f1
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FROUNDI    r4, r3\n"
    "    4: LOAD_IMM   r5, 2147483647.0\n"
    "    5: FCMP       r6, r3, r5, GE\n"
    "    6: LOAD_IMM   r7, 0x7FFFFFFF\n"
    "    7: SELECT     r8, r7, r4, r6\n"
    "    8: ZCAST      r9, r8\n"
    "    9: BITCAST    r10, r3\n"
    "   10: LOAD_IMM   r11, 0x8000000000000000\n"
    "   11: SEQ        r12, r10, r11\n"
    "   12: SLLI       r13, r12, 32\n"
    "   13: LOAD_IMM   r14, 0xFFF8000000000000\n"
    "   14: OR         r15, r14, r13\n"
    "   15: OR         r16, r9, r15\n"
    "   16: BITCAST    r17, r16\n"
    "   17: GET_ALIAS  r18, a2\n"
    "   18: VINSERT    r19, r18, r17, 0\n"
    "   19: VFCAST     r20, r19\n"
    "   20: FNEG       r21, r20\n"
    "   21: VFCAST     r22, r21\n"
    "   22: SET_ALIAS  a2, r22\n"
    "   23: LOAD_IMM   r23, 8\n"
    "   24: SET_ALIAS  a1, r23\n"
    "   25: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,25] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
