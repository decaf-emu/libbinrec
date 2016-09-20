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
    uint32_t reg1, reg2, alias, tempreg[13];
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 1));
    /* Block out every register except EDX through the beginning of the
     * next block.  Note that reg2 holds EAX live through the end of this
     * block due to merging. */
    for (int i = 0; i < lenof(tempreg); i++) {
        EXPECT(tempreg[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, tempreg[i], 0, 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, tempreg[1], 0, 0));  // EDX dies.

    uint32_t reg3, reg4, reg5, reg6, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    for (int i = 0; i < lenof(tempreg); i++) {
        if (i != 1) {  // Skip the register in EDX.
            EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, tempreg[i], 0, 0));
        }
    }
    /* Touch EAX so it can't be used for merging. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 2));
    /* EDX is the only register still available for merging, but we use the
     * loaded value as a divisor so it can't be assigned EDX.  This should
     * trigger a merge through EDX followed by a MOV to the target register. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg4, 0, 0, alias));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MODU, reg6, reg5, reg4, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg6, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC0,                          // xor %eax,%eax
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nop 1(%rip)
    0x33,0xC9,                          // xor %ecx,%ecx
    0x33,0xD2,                          // xor %edx,%edx
    0x33,0xF6,                          // xor %esi,%esi
    0x45,0x33,0xC0,                     // xor %r8d,%r8d
    0x45,0x33,0xC9,                     // xor %r9d,%r9d
    0x45,0x33,0xD2,                     // xor %r10d,%r10d
    0x45,0x33,0xDB,                     // xor %r11d,%r11d
    0x33,0xDB,                          // xor %ebx,%ebx
    0x33,0xED,                          // xor %ebp,%ebp
    0x45,0x33,0xE4,                     // xor %r12d,%r12d
    0x45,0x33,0xED,                     // xor %r13d,%r13d
    0x45,0x33,0xF6,                     // xor %r14d,%r14d
    0x45,0x33,0xFF,                     // xor %r15d,%r15d
    0x48,0x8B,0xD0,                     // mov %rax,%rdx
    0x33,0xC0,                          // xor %eax,%eax
    0x0F,0x1F,0x05,0x02,0x00,0x00,0x00, // nop 2(%rip)
    0x48,0x8B,0xCA,                     // mov %rdx,%rcx
    0x33,0xC0,                          // xor %eax,%eax
    0x33,0xD2,                          // xor %edx,%edx
    0x48,0xF7,0xF1,                     // div %rcx
    0x48,0x89,0x97,0x34,0x12,0x00,0x00, // mov %rdx,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0x41,0x5F,                          // pop %r15
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
