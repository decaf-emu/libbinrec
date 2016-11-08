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
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FZCAST, reg4, reg3, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg5, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0x48,0x03,0xC1,                     // add %rcx,%rax
    0x48,0x85,0xC0,                     // test %rax,%rax
    0x78,0x07,                          // js L1
    0xF3,0x48,0x0F,0x2A,0xC0,           // cvtsi2ss %rax,%xmm0
    0xEB,0x2A,                          // jmp L4
    0x48,0x8B,0xC8,                     // L1: mov %rax,%rcx
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x48,0xD1,0xE9,                     // shr %rcx
    0x0F,0xBA,0x24,0x24,0x0D,           // btl $13,(%rsp)
    0x73,0x10,                          // jnc L3
    0x0F,0xBA,0x24,0x24,0x0E,           // btl $14,(%rsp)
    0x73,0x05,                          // jnc L2
    0xF6,0xC1,0x01,                     // test $1,%cl
    0x74,0x04,                          // jz L3
    0x48,0x83,0xC1,0x01,                // L2: add $1,%rcx
    0xF3,0x48,0x0F,0x2A,0xC1,           // L3: cvtsi2ss %rcx,%xmm0
    0xF3,0x0F,0x58,0xC0,                // addss %xmm0,%xmm0
    0x33,0xC9,                          // L4: xor %ecx,%ecx
    0x48,0x85,0xC0,                     // test %rax,%rax
    0x0F,0x94,0xC1,                     // setz %cl
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
