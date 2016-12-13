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
    0x13,0x81,0x10,0x80,  // ps_cmpu1 cr7,f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: VEXTRACT   r4, r3, 1\n"
    "    4: FGETSTATE  r5\n"
    "    5: FCVT       r6, r4\n"
    "    6: FSETSTATE  r5\n"
    "    7: GET_ALIAS  r7, a3\n"
    "    8: VEXTRACT   r8, r7, 1\n"
    "    9: FGETSTATE  r9\n"
    "   10: FCVT       r10, r8\n"
    "   11: FSETSTATE  r9\n"
    "   12: FCMP       r11, r6, r10, LT\n"
    "   13: FCMP       r12, r6, r10, GT\n"
    "   14: FCMP       r13, r6, r10, EQ\n"
    "   15: FCMP       r14, r6, r10, UN\n"
    "   16: GET_ALIAS  r15, a4\n"
    "   17: SLLI       r16, r11, 3\n"
    "   18: SLLI       r17, r12, 2\n"
    "   19: SLLI       r18, r13, 1\n"
    "   20: OR         r19, r16, r17\n"
    "   21: OR         r20, r18, r14\n"
    "   22: OR         r21, r19, r20\n"
    "   23: BFINS      r22, r15, r21, 0, 4\n"
    "   24: SET_ALIAS  a4, r22\n"
    "   25: GET_ALIAS  r23, a5\n"
    "   26: BFEXT      r24, r23, 12, 7\n"
    "   27: ANDI       r25, r24, 112\n"
    "   28: SLLI       r26, r11, 3\n"
    "   29: SLLI       r27, r12, 2\n"
    "   30: SLLI       r28, r13, 1\n"
    "   31: OR         r29, r26, r27\n"
    "   32: OR         r30, r28, r14\n"
    "   33: OR         r31, r29, r30\n"
    "   34: OR         r32, r25, r31\n"
    "   35: GET_ALIAS  r33, a5\n"
    "   36: BFINS      r34, r33, r32, 12, 7\n"
    "   37: SET_ALIAS  a5, r34\n"
    "   38: FGETSTATE  r35\n"
    "   39: FTESTEXC   r36, r35, INVALID\n"
    "   40: GOTO_IF_Z  r36, L1\n"
    "   41: FCLEAREXC  r37, r35\n"
    "   42: FSETSTATE  r37\n"
    "   43: NOT        r38, r34\n"
    "   44: ORI        r39, r34, 16777216\n"
    "   45: ANDI       r40, r38, 16777216\n"
    "   46: SET_ALIAS  a5, r39\n"
    "   47: GOTO_IF_Z  r40, L2\n"
    "   48: ORI        r41, r39, -2147483648\n"
    "   49: SET_ALIAS  a5, r41\n"
    "   50: LABEL      L2\n"
    "   51: LABEL      L1\n"
    "   52: LOAD_IMM   r42, 4\n"
    "   53: SET_ALIAS  a1, r42\n"
    "   54: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: int32 @ 928(r1)\n"
    "Alias 5: int32 @ 944(r1)\n"
    "\n"
    "Block 0: <none> --> [0,40] --> 1,4\n"
    "Block 1: 0 --> [41,47] --> 2,3\n"
    "Block 2: 1 --> [48,49] --> 3\n"
    "Block 3: 2,1 --> [50,50] --> 4\n"
    "Block 4: 3,0 --> [51,54] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
