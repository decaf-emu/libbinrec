/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include <stdbool.h>

static const binrec_setup_t setup = {
    .guest = BINREC_ARCH_PPC_7XX,
    .host = BINREC_ARCH_PPC_7XX,  // To force host_little_endian to false.
    .host_memory_base = UINT64_C(0x100000000),
    .state_offset_gpr = 0x100,
    .state_offset_fpr = 0x180,
    .state_offset_gqr = 0x380,
    .state_offset_cr = 0x3A0,
    .state_offset_lr = 0x3A4,
    .state_offset_ctr = 0x3A8,
    .state_offset_xer = 0x3AC,
    .state_offset_fpscr = 0x3B0,
    .state_offset_reserve_flag = 0x3B4,
    .state_offset_reserve_state = 0x3B8,
    .state_offset_nia = 0x3BC,
    /* 0x3C0: unused */
    .state_offset_timebase_handler = 0x3C8,
    .state_offset_sc_handler = 0x3D0,
    .state_offset_trap_handler = 0x3D8,
    .state_offset_branch_callback = 0x3E0,
};

static const uint8_t input[] = {
    0x7C,0x64,0x29,0x2D,  // stwcx. r3,r4,r5
};

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: LOAD_U8    r3, 948(r1)\n"
    "    3: LOAD_IMM   r4, 0\n"
    "    4: SET_ALIAS  a5, r4\n"
    "    5: SET_ALIAS  a6, r4\n"
    "    6: SET_ALIAS  a7, r4\n"
    "    7: GET_ALIAS  r5, a10\n"
    "    8: BFEXT      r6, r5, 31, 1\n"
    "    9: SET_ALIAS  a8, r6\n"
    "   10: GOTO_IF_Z  r3, L1\n"
    "   11: STORE_I8   948(r1), r4\n"
    "   12: LOAD       r7, 952(r1)\n"
    "   13: GET_ALIAS  r8, a2\n"
    "   14: GET_ALIAS  r9, a3\n"
    "   15: GET_ALIAS  r10, a4\n"
    "   16: ADD        r11, r9, r10\n"
    "   17: ZCAST      r12, r11\n"
    "   18: ADD        r13, r2, r12\n"
    "   19: CMPXCHG    r14, (r13), r7, r8\n"
    "   20: SEQ        r15, r14, r7\n"
    "   21: GOTO_IF_Z  r15, L1\n"
    "   22: LOAD_IMM   r16, 1\n"
    "   23: SET_ALIAS  a7, r16\n"
    "   24: LABEL      L1\n"
    "   25: LOAD_IMM   r17, 4\n"
    "   26: SET_ALIAS  a1, r17\n"
    "   27: GET_ALIAS  r18, a9\n"
    "   28: ANDI       r19, r18, 268435455\n"
    "   29: GET_ALIAS  r20, a5\n"
    "   30: SLLI       r21, r20, 31\n"
    "   31: OR         r22, r19, r21\n"
    "   32: GET_ALIAS  r23, a6\n"
    "   33: SLLI       r24, r23, 30\n"
    "   34: OR         r25, r22, r24\n"
    "   35: GET_ALIAS  r26, a7\n"
    "   36: SLLI       r27, r26, 29\n"
    "   37: OR         r28, r25, r27\n"
    "   38: GET_ALIAS  r29, a8\n"
    "   39: SLLI       r30, r29, 28\n"
    "   40: OR         r31, r28, r30\n"
    "   41: SET_ALIAS  a9, r31\n"
    "   42: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: int32 @ 276(r1)\n"
    "Alias 5: int32, no bound storage\n"
    "Alias 6: int32, no bound storage\n"
    "Alias 7: int32, no bound storage\n"
    "Alias 8: int32, no bound storage\n"
    "Alias 9: int32 @ 928(r1)\n"
    "Alias 10: int32 @ 940(r1)\n"
    "\n"
    "Block 0: <none> --> [0,10] --> 1,3\n"
    "Block 1: 0 --> [11,21] --> 2,3\n"
    "Block 2: 1 --> [22,23] --> 3\n"
    "Block 3: 2,0,1 --> [24,42] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
