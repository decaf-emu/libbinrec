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
    0x10,0x22,0x20,0xDE,  // ps_madds1 f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FMADDS;
static const unsigned int common_opt = 0;

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
    "    7: VEXTRACT   r8, r4, 1\n"
    "    8: VEXTRACT   r9, r5, 0\n"
    "    9: VEXTRACT   r10, r5, 1\n"
    "   10: FMADD      r11, r6, r8, r9\n"
    "   11: LOAD_IMM   r12, 0.0\n"
    "   12: FCMP       r13, r6, r12, NUN\n"
    "   13: FCMP       r14, r8, r12, UN\n"
    "   14: FCMP       r15, r9, r12, UN\n"
    "   15: AND        r16, r13, r14\n"
    "   16: AND        r17, r16, r15\n"
    "   17: BITCAST    r18, r9\n"
    "   18: LOAD_IMM   r19, 0x8000000000000\n"
    "   19: OR         r20, r18, r19\n"
    "   20: BITCAST    r21, r20\n"
    "   21: SELECT     r22, r21, r11, r17\n"
    "   22: FCVT       r23, r22\n"
    "   23: FMADD      r24, r7, r8, r10\n"
    "   24: LOAD_IMM   r25, 0.0\n"
    "   25: FCMP       r26, r7, r25, NUN\n"
    "   26: FCMP       r27, r8, r25, UN\n"
    "   27: FCMP       r28, r10, r25, UN\n"
    "   28: AND        r29, r26, r27\n"
    "   29: AND        r30, r29, r28\n"
    "   30: BITCAST    r31, r10\n"
    "   31: LOAD_IMM   r32, 0x8000000000000\n"
    "   32: OR         r33, r31, r32\n"
    "   33: BITCAST    r34, r33\n"
    "   34: SELECT     r35, r34, r24, r30\n"
    "   35: FCVT       r36, r35\n"
    "   36: VBUILD2    r37, r23, r36\n"
    "   37: VFCVT      r38, r37\n"
    "   38: SET_ALIAS  a2, r38\n"
    "   39: LOAD_IMM   r39, 4\n"
    "   40: SET_ALIAS  a1, r39\n"
    "   41: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,41] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
