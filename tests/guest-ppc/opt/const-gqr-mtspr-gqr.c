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
    0x7C,0x61,0xE3,0xA6,  // mtspr 897,r3
    /* Scanning stops here because of the store to a GQR. */
    0x38,0x60,0x00,0x00,  // li r3,0
};

#define INITIAL_STATE \
    &(PPCInsnTestState){.gqr = {0}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_CONSTANT_GQRS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: STORE      900(r1), r3\n"
    "    4: LOAD_IMM   r4, 4\n"
    "    5: SET_ALIAS  a1, r4\n"
    "    6: GOTO       L1\n"
    "    7: LOAD_IMM   r5, 4\n"
    "    8: SET_ALIAS  a1, r5\n"
    "    9: LABEL      L1\n"
    "   10: RETURN\n"
    "\n"
    "Alias 1: int32 @ 956(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "\n"
    "Block 0: <none> --> [0,6] --> 2\n"
    "Block 1: <none> --> [7,8] --> 2\n"
    "Block 2: 1,0 --> [9,10] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
