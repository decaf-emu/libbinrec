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
    0xC0,0x63,0x00,0x00,  // lfs f3,0(r3)
    0xC0,0x83,0x00,0x04,  // lfs f4,4(r3)
    0x10,0x22,0x20,0xEE,  // ps_sel f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_ASSUME_NO_SNAN;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, 0(r5)\n"
    "    6: ZCAST      r7, r3\n"
    "    7: ADD        r8, r2, r7\n"
    "    8: LOAD_BR    r9, 4(r8)\n"
    "    9: FGETSTATE  r10\n"
    "   10: GET_ALIAS  r11, a4\n"
    "   11: VBROADCAST r12, r6\n"
    "   12: VBROADCAST r13, r9\n"
    "   13: LOAD_IMM   r14, 0.0\n"
    "   14: VEXTRACT   r15, r11, 0\n"
    "   15: VEXTRACT   r16, r12, 0\n"
    "   16: VEXTRACT   r17, r13, 0\n"
    "   17: FCMP       r18, r15, r14, GE\n"
    "   18: SELECT     r19, r16, r17, r18\n"
    "   19: VEXTRACT   r20, r11, 1\n"
    "   20: VEXTRACT   r21, r12, 1\n"
    "   21: VEXTRACT   r22, r13, 1\n"
    "   22: FCMP       r23, r20, r14, GE\n"
    "   23: SELECT     r24, r21, r22, r23\n"
    "   24: VBUILD2    r25, r19, r24\n"
    "   25: FSETSTATE  r10\n"
    "   26: VFCVT      r26, r25\n"
    "   27: SET_ALIAS  a3, r26\n"
    "   28: FCVT       r27, r6\n"
    "   29: VBROADCAST r28, r27\n"
    "   30: SET_ALIAS  a5, r28\n"
    "   31: FCVT       r29, r9\n"
    "   32: VBROADCAST r30, r29\n"
    "   33: SET_ALIAS  a6, r30\n"
    "   34: LOAD_IMM   r31, 12\n"
    "   35: SET_ALIAS  a1, r31\n"
    "   36: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2] @ 416(r1)\n"
    "Alias 5: float64[2] @ 432(r1)\n"
    "Alias 6: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,36] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
