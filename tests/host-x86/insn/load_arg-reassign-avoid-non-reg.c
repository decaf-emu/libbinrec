/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/common.h"
#include "tests/host-x86/common.h"


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_WINDOWS,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    /* This violates the rule on LOAD_ARG coming first in the unit, but
     * it's useful to force the input argument to be moved. */
    alloc_dummy_registers(unit, 2, RTLTYPE_INT32);

    int reg1, reg2;
    /* The first argument is passed in rCX (on Windows), but it's not
     * available here, so the value should be moved to the next available
     * register (rDX). */
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    /* Check that the clobber scan loop doesn't choke on a LOAD_ARG for a
     * non-register argument.  (Such a LOAD_ARG will cause translation
     * failure when it's later encountered, but we shouldn't fail before
     * that point.) */
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 4));

    return EXIT_SUCCESS;
}

#define EXPECT_TRANSLATE_FAILURE

static const char expected_log[] =
    "[error] LOAD_ARG 4 not supported (argument is not in a register)\n"
    ;

#include "tests/rtl-translate-test.i"
