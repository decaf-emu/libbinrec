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
    0xFF,0x81,0x10,0x40,  // fcmpo cr7,f1,f2
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_IGNORE_FPSCR_VXFOO;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a9\n"
    "    3: BFEXT      r4, r3, 12, 7\n"
    "    4: SET_ALIAS  a10, r4\n"
    "    5: GET_ALIAS  r5, a2\n"
    "    6: GET_ALIAS  r6, a3\n"
    "    7: FCMP       r7, r5, r6, OLT\n"
    "    8: FCMP       r8, r5, r6, OGT\n"
    "    9: FCMP       r9, r5, r6, OEQ\n"
    "   10: FCMP       r10, r5, r6, OUN\n"
    "   11: SET_ALIAS  a4, r7\n"
    "   12: SET_ALIAS  a5, r8\n"
    "   13: SET_ALIAS  a6, r9\n"
    "   14: SET_ALIAS  a7, r10\n"
    "   15: GET_ALIAS  r11, a10\n"
    "   16: ANDI       r12, r11, 112\n"
    "   17: SLLI       r13, r7, 3\n"
    "   18: SLLI       r14, r8, 2\n"
    "   19: SLLI       r15, r9, 1\n"
    "   20: OR         r16, r13, r14\n"
    "   21: OR         r17, r15, r10\n"
    "   22: OR         r18, r16, r17\n"
    "   23: OR         r19, r12, r18\n"
    "   24: SET_ALIAS  a10, r19\n"
    "   25: FGETSTATE  r20\n"
    "   26: FTESTEXC   r21, r20, INVALID\n"
    "   27: GOTO_IF_Z  r21, L1\n"
    "   28: FCLEAREXC\n"
    "   29: GET_ALIAS  r22, a9\n"
    "   30: ORI        r23, r22, 17301504\n"
    "   31: SET_ALIAS  a9, r23\n"
    "   32: LABEL      L1\n"
    "   33: LOAD_IMM   r24, 4\n"
    "   34: SET_ALIAS  a1, r24\n"
    "   35: GET_ALIAS  r25, a8\n"
    "   36: ANDI       r26, r25, -16\n"
    "   37: GET_ALIAS  r27, a4\n"
    "   38: SLLI       r28, r27, 3\n"
    "   39: OR         r29, r26, r28\n"
    "   40: GET_ALIAS  r30, a5\n"
    "   41: SLLI       r31, r30, 2\n"
    "   42: OR         r32, r29, r31\n"
    "   43: GET_ALIAS  r33, a6\n"
    "   44: SLLI       r34, r33, 1\n"
    "   45: OR         r35, r32, r34\n"
    "   46: GET_ALIAS  r36, a7\n"
    "   47: OR         r37, r35, r36\n"
    "   48: SET_ALIAS  a8, r37\n"
    "   49: GET_ALIAS  r38, a9\n"
    "   50: GET_ALIAS  r39, a10\n"
    "   51: ANDI       r40, r38, -1611134977\n"
    "   52: SLLI       r41, r39, 12\n"
    "   53: OR         r42, r40, r41\n"
    "   54: SET_ALIAS  a9, r42\n"
    "   55: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: float64 @ 400(r1)\n"
    "Alias 3: float64 @ 416(r1)\n"
    "Alias 4: int32, no bound storage\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32 @ 928(r1)\n"
    "Alias 9: int32 @ 944(r1)\n"
    "Alias 10: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,27] --> 1,2\n"
    "Block 1: 0 --> [28,31] --> 2\n"
    "Block 2: 1,0 --> [32,55] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
