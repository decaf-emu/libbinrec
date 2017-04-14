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
    0x7C,0x61,0x14,0x2A,  // lswx r3,r1,r2
};

static const unsigned int guest_opt = 0;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ZCAST      r4, r3\n"
    "    4: ADD        r5, r2, r4\n"
    "    5: GET_ALIAS  r6, a4\n"
    "    6: ZCAST      r7, r6\n"
    "    7: ADD        r8, r5, r7\n"
    "    8: GET_ALIAS  r9, a34\n"
    "    9: ANDI       r10, r9, 127\n"
    "   10: ANDI       r11, r10, 3\n"
    "   11: GOTO_IF_Z  r11, L1\n"
    "   12: ADDI       r12, r10, 12\n"
    "   13: ANDI       r13, r12, 124\n"
    "   14: ZCAST      r14, r13\n"
    "   15: ADD        r15, r1, r14\n"
    "   16: LOAD_IMM   r16, 0\n"
    "   17: STORE      256(r15), r16\n"
    "   18: LABEL      L1\n"
    "   19: ZCAST      r17, r10\n"
    "   20: LOAD_IMM   r18, 0x0\n"
    "   21: SET_ALIAS  a35, r18\n"
    "   22: LOAD_IMM   r19, 0xC\n"
    "   23: SET_ALIAS  a36, r19\n"
    "   24: LABEL      L2\n"
    "   25: GET_ALIAS  r20, a35\n"
    "   26: SLTU       r21, r20, r17\n"
    "   27: GOTO_IF_Z  r21, L3\n"
    "   28: GET_ALIAS  r22, a36\n"
    "   29: ADD        r23, r8, r20\n"
    "   30: XORI       r24, r22, 3\n"
    "   31: ADD        r25, r1, r24\n"
    "   32: LOAD_U8    r26, 0(r23)\n"
    "   33: STORE_I8   256(r25), r26\n"
    "   34: ADDI       r27, r20, 1\n"
    "   35: SET_ALIAS  a35, r27\n"
    "   36: ADDI       r28, r22, 1\n"
    "   37: ANDI       r29, r28, 127\n"
    "   38: SET_ALIAS  a36, r29\n"
    "   39: GOTO       L2\n"
    "   40: LABEL      L3\n"
    "   41: LOAD_IMM   r30, 4\n"
    "   42: SET_ALIAS  a1, r30\n"
    "   43: RETURN\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
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
    "Block 0: <none> --> [0,11] --> 1,2\n"
    "Block 1: 0 --> [12,17] --> 2\n"
    "Block 2: 1,0 --> [18,23] --> 3\n"
    "Block 3: 2,4 --> [24,27] --> 4,5\n"
    "Block 4: 3 --> [28,39] --> 3\n"
    "Block 5: 3 --> [40,43] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
