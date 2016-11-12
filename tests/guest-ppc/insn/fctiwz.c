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
    0xFC,0x20,0x10,0x1E,  // fctiwz f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: FTRUNCI    r4, r3\n"
    "    4: ZCAST      r5, r4\n"
    "    5: BITCAST    r6, r5\n"
    "    6: FGETSTATE  r7\n"
    "    7: FTESTEXC   r8, r7, INVALID\n"
    "    8: GOTO_IF_Z  r8, L1\n"
    "    9: LOAD_IMM   r9, 0.0\n"
    "   10: FCMP       r10, r3, r9, GT\n"
    "   11: GOTO_IF_Z  r10, L1\n"
    "   12: LOAD_IMM   r11, 1.0609978949885705e-314\n"
    "   13: SET_ALIAS  a2, r11\n"
    "   14: GOTO       L2\n"
    "   15: LABEL      L1\n"
    "   16: SET_ALIAS  a2, r6\n"
    "   17: LABEL      L2\n"
    "   18: FCLEAREXC  r12, r7\n"
    "   19: FSETSTATE  r12\n"
    "   20: LOAD_IMM   r13, 4\n"
    "   21: SET_ALIAS  a1, r13\n"
    "   22: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> 1,3\n"
    "Block 1: 0 --> [9,11] --> 2,3\n"
    "Block 2: 1 --> [12,14] --> 4\n"
    "Block 3: 0,1 --> [15,16] --> 4\n"
    "Block 4: 3,2 --> [17,22] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
