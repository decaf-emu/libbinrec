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
static const unsigned int host_opt = BINREC_OPT_H_X86_FIXED_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));  // reg1=EAX
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));  // reg2=ECX
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_MODU, reg3, reg1, reg2, 0));  // reg3=EDX

    /* reg4 should not get EDX here due to the dest != src2 constraint. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_MODU, reg4, reg1, reg3, 0));  // reg4=ECX

    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0));  // reg5=ECX
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg1=EAX should not keep reg6 from getting EDX. */
    EXPECT(rtl_add_insn(unit, RTLOP_MODU, reg6, reg5, reg1, 0));  // reg6=EDX

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC0,                          // xor %eax,%eax
    0x33,0xC9,                          // xor %ecx,%ecx
    0x48,0x8B,0xF0,                     // mov %rax,%rsi
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF1,                          // div %ecx
    0x48,0x8B,0xC6,                     // mov %rsi,%rax
    0x48,0x8B,0xF0,                     // mov %rax,%rsi
    0x48,0x8B,0xCA,                     // mov %rdx,%rcx
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF1,                          // div %ecx
    0x48,0x87,0xD1,                     // xchg %rdx,%rcx
    0x48,0x8B,0xC6,                     // mov %rsi,%rax
    0x33,0xC9,                          // xor %ecx,%ecx
    0x48,0x8B,0xF0,                     // mov %rax,%rsi
    0x8B,0xC1,                          // mov %ecx,%eax
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF6,                          // div %esi
    0x48,0x8B,0xC6,                     // mov %rsi,%rax
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
