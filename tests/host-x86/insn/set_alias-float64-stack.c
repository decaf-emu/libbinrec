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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 3, RTLTYPE_FLOAT32);

    int reg1, alias1, alias2;
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_FLOAT64));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x4000000000000000)));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias2));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x18,                // sub $24,%rsp
    0x48,0xB9,0x00,0x00,0x00,0x00,0x00, // mov $0x4000000000000000,%rcx
      0x00,0x00,0x40,
    0x66,0x48,0x0F,0x6E,0xD9,           // movq %rcx,%xmm3
    0xF2,0x0F,0x11,0x5C,0x24,0x08,      // movsd %xmm3,8(%rsp)
    0x48,0x83,0xC4,0x18,                // add $24,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
