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

    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FGETSTATE, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETROUND,
                        reg2, reg1, 0, RTLFROUND_NEAREST));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETROUND,
                        reg3, reg1, 0, RTLFROUND_TRUNC));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETROUND,
                        reg4, reg1, 0, RTLFROUND_FLOOR));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FPSTATE));
    EXPECT(rtl_add_insn(unit, RTLOP_FSETROUND,
                        reg5, reg1, 0, RTLFROUND_CEIL));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0x8B,0xD1,                          // mov %ecx,%edx
    0x81,0xE2,0xFF,0x9F,0x00,0x00,      // and $0x9FFF,%edx
    0x8B,0xD1,                          // mov %ecx,%edx
    0x81,0xE2,0xFF,0x9F,0x00,0x00,      // and $0x9FFF,%edx
    0x81,0xCA,0x00,0x60,0x00,0x00,      // or $0x6000,%edx
    0x8B,0xD1,                          // mov %ecx,%edx
    0x81,0xE2,0xFF,0x9F,0x00,0x00,      // and $0x9FFF,%edx
    0x81,0xCA,0x00,0x20,0x00,0x00,      // or $0x2000,%edx
    0x8B,0xD1,                          // mov %ecx,%edx
    0x81,0xE2,0xFF,0x9F,0x00,0x00,      // and $0x9FFF,%edx
    0x81,0xCA,0x00,0x40,0x00,0x00,      // or $0x4000,%edx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
