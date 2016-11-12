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
    0x7C,0x61,0x15,0x2A,  // stswx r3,r1,r2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: ZCAST      r7, r6\n"
    "    7: ADD        r8, r5, r7\n"
    "    8: GET_ALIAS  r9, a34\n"
    "    9: ANDI       r10, r9, 127\n"
    "   10: ZCAST      r11, r10\n"
    "   11: LOAD_IMM   r12, 0x0\n"
    "   12: SET_ALIAS  a35, r12\n"
    "   13: LOAD_IMM   r13, 0xC\n"
    "   14: SET_ALIAS  a36, r13\n"
    "   15: LABEL      L1\n"
    "   16: GET_ALIAS  r14, a35\n"
    "   17: SLTU       r15, r14, r11\n"
    "   18: GOTO_IF_Z  r15, L2\n"
    "   19: GET_ALIAS  r16, a36\n"
    "   20: ADD        r17, r8, r14\n"
    "   21: XORI       r18, r16, 3\n"
    "   22: ADD        r19, r1, r18\n"
    "   23: LOAD_U8    r20, 256(r19)\n"
    "   24: STORE_I8   0(r17), r20\n"
    "   25: ADDI       r21, r14, 1\n"
    "   26: SET_ALIAS  a35, r21\n"
    "   27: ADDI       r22, r16, 1\n"
    "   28: ANDI       r23, r22, 127\n"
    "   29: SET_ALIAS  a36, r23\n"
    "   30: GOTO       L1\n"
    "   31: LABEL      L2\n"
    "   32: LOAD_IMM   r24, 4\n"
    "   33: SET_ALIAS  a1, r24\n"
    "   34: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 260(r1)\n"
    "Alias 4: int32 @ 264(r1)\n"
    "Alias 5: int32 @ 268(r1)\n"
    "Alias 6: int32 @ 272(r1)\n"
    "Alias 7: int32 @ 276(r1)\n"
    "Alias 8: int32 @ 280(r1)\n"
    "Alias 9: int32 @ 284(r1)\n"
    "Alias 10: int32 @ 288(r1)\n"
    "Alias 11: int32 @ 292(r1)\n"
    "Alias 12: int32 @ 296(r1)\n"
    "Alias 13: int32 @ 300(r1)\n"
    "Alias 14: int32 @ 304(r1)\n"
    "Alias 15: int32 @ 308(r1)\n"
    "Alias 16: int32 @ 312(r1)\n"
    "Alias 17: int32 @ 316(r1)\n"
    "Alias 18: int32 @ 320(r1)\n"
    "Alias 19: int32 @ 324(r1)\n"
    "Alias 20: int32 @ 328(r1)\n"
    "Alias 21: int32 @ 332(r1)\n"
    "Alias 22: int32 @ 336(r1)\n"
    "Alias 23: int32 @ 340(r1)\n"
    "Alias 24: int32 @ 344(r1)\n"
    "Alias 25: int32 @ 348(r1)\n"
    "Alias 26: int32 @ 352(r1)\n"
    "Alias 27: int32 @ 356(r1)\n"
    "Alias 28: int32 @ 360(r1)\n"
    "Alias 29: int32 @ 364(r1)\n"
    "Alias 30: int32 @ 368(r1)\n"
    "Alias 31: int32 @ 372(r1)\n"
    "Alias 32: int32 @ 376(r1)\n"
    "Alias 33: int32 @ 380(r1)\n"
    "Alias 34: int32 @ 940(r1)\n"
    "Alias 35: address, no bound storage\n"
    "Alias 36: address, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,14] --> 1\n"
    "Block 1: 0,2 --> [15,18] --> 2,3\n"
    "Block 2: 1 --> [19,30] --> 1\n"
    "Block 3: 1 --> [31,34] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
