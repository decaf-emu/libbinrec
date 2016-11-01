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
static const unsigned int host_opt = BINREC_OPT_H_X86_CONDITION_CODES;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg4, reg1, reg2, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg5, reg3, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg6, reg4, reg4, 0));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg7, reg6, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0x8B,0xD0,                          // mov %eax,%edx
    0x03,0xD1,                          // add %ecx,%edx
    0x48,0x8B,0xF2,                     // mov %rdx,%rsi
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF1,                          // div %ecx
    0x48,0x8B,0xD6,                     // mov %rsi,%rdx
    0x33,0xC9,                          // xor %ecx,%ecx
    0x85,0xD2,                          // test %edx,%edx
    0x0F,0x94,0xC1,                     // setz %cl
    0x48,0x8B,0xC8,                     // mov %rax,%rcx
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF1,                          // div %ecx
    0x48,0x87,0xC1,                     // xchg %rax,%rcx
    0x33,0xC0,                          // xor %eax,%eax
    0x85,0xC9,                          // test %ecx,%ecx
    0x0F,0x94,0xC0,                     // setz %al
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
