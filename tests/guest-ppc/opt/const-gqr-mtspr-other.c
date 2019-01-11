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
    0x7C,0x69,0x03,0xA6,  // mtctr r3 (mtspr 9,r3)
    /* Scanning should not stop here because the mtspr target is not a GQR. */
    0x38,0x60,0x00,0x00,  // li r3,0
};

#define INITIAL_STATE \
    &(PPCInsnTestState){.gqr = {0}}

static const unsigned int guest_opt = BINREC_OPT_G_PPC_CONSTANT_GQRS;
static const unsigned int common_opt = 0;

static const bool expected_success = true;

static const char expected[] =
    "[info] Scanning terminated at requested limit 0x7\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a2\n"
    "    3: SET_ALIAS  a3, r3\n"
    "    4: LOAD_IMM   r4, 0\n"
    "    5: SET_ALIAS  a2, r4\n"
    "    6: LOAD_IMM   r5, 8\n"
    "    7: SET_ALIAS  a1, r5\n"
    "    8: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: int32 @ 268(r1)\n"
    "Alias 3: int32 @ 936(r1)\n"
    "\n"
    "Block 0: <none> --> [0,8] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
