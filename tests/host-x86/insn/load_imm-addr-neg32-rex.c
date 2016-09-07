/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/host-x86/common.h"


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_SYSV,
};

static bool add_rtl(RTLUnit *unit)
{
    EXPECT(alloc_dummy_registers(unit, 9, RTLTYPE_INT32));

    uint32_t reg;
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0,
                        UINT64_C(0xFFFFFFFF80000000)));

    return true;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x49,0xC7,0xC2,0x00,0x00,0x00,0x80, // mov $-0x80000000,%r10
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
