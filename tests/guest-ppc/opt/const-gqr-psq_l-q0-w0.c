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
    0xE0,0x23,0x2F,0xF0,  // psq_l f1,-16(r3),0,2
};

#define INITIAL_STATE \
    &(PPCInsnTestState){.gqr = {-1,-1, 0x00000000, -1,-1,-1,-1,-1}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_CONSTANT_GQRS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD_BR    r6, -16(r5)\n"
    "    6: LOAD_BR    r7, -12(r5)\n"
    "    7: VBUILD2    r8, r6, r7\n"
    "    8: FGETSTATE  r9\n"
    "    9: VFCMP      r10, r8, r8, UN\n"
    "   10: VFCVT      r11, r8\n"
    "   11: SET_ALIAS  a4, r11\n"
    "   12: GOTO_IF_Z  r10, L1\n"
    "   13: VEXTRACT   r12, r8, 0\n"
    "   14: VEXTRACT   r13, r8, 1\n"
    "   15: SLLI       r14, r10, 32\n"
    "   16: BITCAST    r15, r12\n"
    "   17: BITCAST    r16, r13\n"
    "   18: NOT        r17, r15\n"
    "   19: NOT        r18, r16\n"
    "   20: ANDI       r19, r17, 4194304\n"
    "   21: ANDI       r20, r18, 4194304\n"
    "   22: VEXTRACT   r21, r11, 0\n"
    "   23: VEXTRACT   r22, r11, 1\n"
    "   24: ZCAST      r23, r19\n"
    "   25: ZCAST      r24, r20\n"
    "   26: SLLI       r25, r23, 29\n"
    "   27: SLLI       r26, r24, 29\n"
    "   28: BITCAST    r27, r21\n"
    "   29: BITCAST    r28, r22\n"
    "   30: AND        r29, r25, r14\n"
    "   31: AND        r30, r26, r10\n"
    "   32: XOR        r31, r27, r29\n"
    "   33: XOR        r32, r28, r30\n"
    "   34: BITCAST    r33, r31\n"
    "   35: BITCAST    r34, r32\n"
    "   36: VBUILD2    r35, r33, r34\n"
    "   37: SET_ALIAS  a4, r35\n"
    "   38: LABEL      L1\n"
    "   39: GET_ALIAS  r36, a4\n"
    "   40: FSETSTATE  r9\n"
    "   41: SET_ALIAS  a3, r36\n"
    "   42: LOAD_IMM   r37, 4\n"
    "   43: SET_ALIAS  a1, r37\n"
    "   44: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: float64[2] @ 400(r1)\n"
    "Alias 4: float64[2], no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1,2\n"
    "Block 1: 0 --> [13,37] --> 2\n"
    "Block 2: 1,0 --> [38,44] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
