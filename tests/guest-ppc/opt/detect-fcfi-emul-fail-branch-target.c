/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/guest-ppc/insn/common.h"

#define ADD_READONLY_REGION

static const uint8_t input[] = {
    0x94,0x21,0xFF,0xF0,  // stwu r1,-16(r1)
    0x3C,0x00,0x43,0x30,  // lis r0,0x4330
    0x48,0x00,0x00,0x08,  // b 0x10
    0x90,0x01,0x00,0x08,  // stw r0,8(r1)
    /* The existence of a branch target here should block optimization of
     * the fsub. */
    0x90,0x61,0x00,0x0C,  // stw r3,12(r1)
    0xC8,0x41,0x00,0x08,  // lfd f2,8(r1)
    0x3C,0x60,0x00,0x00,  // lis r3,0
    0xC8,0x63,0x00,0x2C,  // lfd f3,const(r3)
    0xFC,0x22,0x18,0x28,  // fsub f1,f2,f3
    0x38,0x21,0x00,0x10,  // addi r1,r1,16
    0x4E,0x80,0x00,0x20,  // blr
    0x43,0x30,0x00,0x00,  // const: .int64 0x4330000000000000
      0x00,0x00,0x00,0x00,
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_DETECT_FCFI_EMUL
                                    | BINREC_OPT_G_PPC_NO_FPSCR_STATE;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Failed to optimize possible fcfi at 0x20: stores to lfd buffer not found\n"
    #endif
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: ADDI       r4, r3, -16\n"
    "    4: ZCAST      r5, r4\n"
    "    5: ADD        r6, r2, r5\n"
    "    6: STORE_BR   0(r6), r3\n"
    "    7: SET_ALIAS  a3, r4\n"
    "    8: LOAD_IMM   r7, 0x43300000\n"
    "    9: SET_ALIAS  a2, r7\n"
    "   10: GOTO       L1\n"
    "   11: LOAD_IMM   r8, 12\n"
    "   12: SET_ALIAS  a1, r8\n"
    "   13: GET_ALIAS  r9, a3\n"
    "   14: ZCAST      r10, r9\n"
    "   15: ADD        r11, r2, r10\n"
    "   16: GET_ALIAS  r12, a2\n"
    "   17: STORE_BR   8(r11), r12\n"
    "   18: LOAD_IMM   r13, 16\n"
    "   19: SET_ALIAS  a1, r13\n"
    "   20: LABEL      L1\n"
    "   21: GET_ALIAS  r14, a3\n"
    "   22: ZCAST      r15, r14\n"
    "   23: ADD        r16, r2, r15\n"
    "   24: GET_ALIAS  r17, a4\n"
    "   25: STORE_BR   12(r16), r17\n"
    "   26: ZCAST      r18, r14\n"
    "   27: ADD        r19, r2, r18\n"
    "   28: LOAD_BR    r20, 8(r19)\n"
    "   29: LOAD_IMM   r21, 0\n"
    "   30: SET_ALIAS  a4, r21\n"
    "   31: ZCAST      r22, r21\n"
    "   32: ADD        r23, r2, r22\n"
    "   33: LOAD_BR    r24, 44(r23)\n"
    "   34: FSUB       r25, r20, r24\n"
    "   35: ADDI       r26, r14, 16\n"
    "   36: SET_ALIAS  a3, r26\n"
    "   37: SET_ALIAS  a5, r25\n"
    "   38: SET_ALIAS  a6, r20\n"
    "   39: SET_ALIAS  a7, r24\n"
    "   40: GET_ALIAS  r27, a8\n"
    "   41: ANDI       r28, r27, -4\n"
    "   42: SET_ALIAS  a1, r28\n"
    "   43: GOTO       L2\n"
    "   44: LOAD_IMM   r29, 44\n"
    "   45: SET_ALIAS  a1, r29\n"
    "   46: LABEL      L2\n"
    "   47: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 256(r1)\n"
    "Alias 3: int32 @ 260(r1)\n"
    "Alias 4: int32 @ 268(r1)\n"
    "Alias 5: float64 @ 400(r1)\n"
    "Alias 6: float64 @ 416(r1)\n"
    "Alias 7: float64 @ 432(r1)\n"
    "Alias 8: int32 @ 932(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 2\n"
    "Block 1: <none> --> [11,19] --> 2\n"
    "Block 2: 1,0 --> [20,43] --> 4\n"
    "Block 3: <none> --> [44,45] --> 4\n"
    "Block 4: 3,2 --> [46,47] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
