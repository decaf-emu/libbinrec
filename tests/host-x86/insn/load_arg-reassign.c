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

    int reg1;
    /* The first argument is passed in rCX (on Windows), but it's not
     * available here, so the value should be moved to the next available
     * register (rDX). */
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    /* Check that the clobber scan loop doesn't go past the end of the
     * instruction array. */
    ASSERT(unit->num_insns + 1 <= unit->insns_size);
    unit->insns[unit->num_insns].opcode = RTLOP_LOAD_ARG;
    unit->insns[unit->num_insns].arg_index = 1;

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x8B,0xD1,                          // mov %ecx,%edx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
