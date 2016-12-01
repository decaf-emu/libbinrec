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
    0x7C,0x60,0x20,0x28,  // lwarx r3,0,r4
    0x7C,0xA0,0x21,0x2D,  // stwcx. r5,0,r4
    0x41,0x82,0xFF,0xFC,  // beq 0x4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_PAIRED_LWARX_STWCX;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0xB\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: LOAD       r6, 0(r5)\n"
    "    6: BSWAP      r7, r6\n"
    "    7: LOAD_IMM   r8, 1\n"
    "    8: STORE_I8   948(r1), r8\n"
    "    9: STORE      952(r1), r6\n"
    "   10: SET_ALIAS  a2, r7\n"
    "   11: LOAD_IMM   r9, 4\n"
    "   12: SET_ALIAS  a1, r9\n"
    "   13: LABEL      L1\n"
    "   14: LOAD_U8    r10, 948(r1)\n"
    "   15: LOAD_IMM   r11, 0\n"
    "   16: GET_ALIAS  r12, a6\n"
    "   17: BFEXT      r13, r12, 31, 1\n"
    "   18: GET_ALIAS  r14, a5\n"
    "   19: BFINS      r15, r14, r13, 28, 4\n"
    "   20: SET_ALIAS  a5, r15\n"
    "   21: GOTO_IF_Z  r10, L2\n"
    "   22: STORE_I8   948(r1), r11\n"
    "   23: LOAD       r16, 952(r1)\n"
    "   24: GET_ALIAS  r17, a4\n"
    "   25: BSWAP      r18, r17\n"
    "   26: GET_ALIAS  r19, a3\n"
    "   27: ZCAST      r20, r19\n"
    "   28: ADD        r21, r2, r20\n"
    "   29: CMPXCHG    r22, (r21), r16, r18\n"
    "   30: SEQ        r23, r22, r16\n"
    "   31: GOTO_IF_Z  r23, L2\n"
    "   32: GET_ALIAS  r24, a5\n"
    "   33: ORI        r25, r24, 536870912\n"
    "   34: SET_ALIAS  a5, r25\n"
    "   35: LABEL      L2\n"
    "   36: GET_ALIAS  r26, a5\n"
    "   37: ANDI       r27, r26, 536870912\n"
    "   38: GOTO_IF_NZ r27, L1\n"
    "   39: LOAD_IMM   r28, 12\n"
    "   40: SET_ALIAS  a1, r28\n"
    "   41: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32 @ 928(r1)\n"
    "Alias 6: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,12] --> 1\n"
    "Block 1: 0,4 --> [13,21] --> 2,4\n"
    "Block 2: 1 --> [22,31] --> 3,4\n"
    "Block 3: 2 --> [32,34] --> 4\n"
    "Block 4: 3,1,2 --> [35,38] --> 5,1\n"
    "Block 5: 4 --> [39,41] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
