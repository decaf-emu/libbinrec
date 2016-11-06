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
    0x7C,0x23,0x27,0xAE,  // stfiwx f1,r3,r4
};

static const unsigned int guest_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_IMM   r2, 0x100000000\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: GET_ALIAS  r4, a3\n"
    "    4: ADD        r5, r3, r4\n"
    "    5: ZCAST      r6, r5\n"
    "    6: ADD        r7, r2, r6\n"
    "    7: GET_ALIAS  r8, a4\n"
    "    8: BITCAST    r9, r8\n"
    "    9: ZCAST      r10, r9\n"
    "   10: STORE      0(r7), r10\n"
    "   11: LOAD_IMM   r11, 4\n"
    "   12: SET_ALIAS  a1, r11\n"
    "   13: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 272(r1)\n"
    "Alias 4: float64 @ 400(r1)\n"
    "\n"
    "Block 0: <none> --> [0,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
