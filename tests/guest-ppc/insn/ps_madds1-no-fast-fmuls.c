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
    0x10,0x22,0x20,0xDE,  // ps_madds1 f1,f2,f3,f4
};

static const unsigned int guest_opt = BINREC_OPT_G_PPC_NO_FPSCR_STATE
                                    | BINREC_OPT_G_PPC_FAST_FMADDS;
static const unsigned int common_opt = BINREC_OPT_NATIVE_IEEE_NAN;

static const bool expected_success = true;

/* Disabling FAST_FMULS has no effect on ps_madds1 because the ps1 slot
 * is assumed to have already been converted to single precision. */
static const char expected[] =
    "[info] Scanning terminated at requested limit 0x3\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: GET_ALIAS  r3, a3\n"
    "    3: GET_ALIAS  r4, a4\n"
    "    4: GET_ALIAS  r5, a5\n"
    "    5: VEXTRACT   r6, r4, 1\n"
    "    6: VBROADCAST r7, r6\n"
    "    7: FMADD      r8, r3, r7, r5\n"
    "    8: VFCVT      r9, r8\n"
    "    9: VFCVT      r10, r9\n"
    "   10: SET_ALIAS  a2, r10\n"
    "   11: LOAD_IMM   r11, 4\n"
    "   12: SET_ALIAS  a1, r11\n"
    "   13: RETURN     r1\n"
    "\n"
    "Alias 1: int32 @ 964(r1)\n"
    "Alias 2: float64[2] @ 400(r1)\n"
    "Alias 3: float64[2] @ 416(r1)\n"
    "Alias 4: float64[2] @ 432(r1)\n"
    "Alias 5: float64[2] @ 448(r1)\n"
    "\n"
    "Block 0: <none> --> [0,13] --> <none>\n"
    ;

#include "tests/rtl-disasm-test.i"
