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

    uint32_t reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg2, reg1, 0, -128));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg3, reg1, 0, 127));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg4, reg1, 0, -129));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg5, reg1, 0, 128));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC9,                          // xor %ecx,%ecx
    0x83,0xF9,0x80,                     // cmp $-128,%ecx
    0x0F,0x94,0xC2,                     // sete %dl
    0x0F,0xB6,0xD2,                     // movzbl %dl,%edx
    0x83,0xF9,0x7F,                     // cmp $127,%ecx
    0x0F,0x94,0xC2,                     // sete %dl
    0x0F,0xB6,0xD2,                     // movzbl %dl,%edx
    0x81,0xF9,0x7F,0xFF,0xFF,0xFF,      // cmp $-129,%ecx
    0x0F,0x94,0xC2,                     // sete %dl
    0x0F,0xB6,0xD2,                     // movzbl %dl,%edx
    0x81,0xF9,0x80,0x00,0x00,0x00,      // cmp $128,%ecx
    0x0F,0x94,0xC1,                     // sete %cl
    0x0F,0xB6,0xC9,                     // movzbl %cl,%ecx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"