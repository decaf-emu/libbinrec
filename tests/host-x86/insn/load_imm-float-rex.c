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
    .host = BINREC_ARCH_X86_64_SYSV,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 6, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 8, RTLTYPE_FLOAT);

    int reg;
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, 0x3F800000));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x41,0xB9,0x00,0x00,0x80,0x3F,      // mov $0x3F800000,%r9d
    0x66,0x45,0x0F,0x6E,0xC1,           // movd %r9d,%xmm8
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
