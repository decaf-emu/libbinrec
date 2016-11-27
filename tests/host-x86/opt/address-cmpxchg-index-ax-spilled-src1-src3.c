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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg3, 0, 0, 1));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg4, 0, 0, 2));

    int spillers[12];
    for (int i = 0; i < lenof(spillers); i++) {
        EXPECT(spillers[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, spillers[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(spillers); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, spillers[i], 0, 0));
    }

    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg5, reg1, reg2, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CMPXCHG, reg6, reg5, reg3, reg4));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x83,0xEC,0x18,                // sub $24,%rsp
    0x48,0x8B,0xC7,                     // mov %rdi,%rax
    0x48,0x83,0xC0,0x01,                // addi $1,%rax
    0x89,0x14,0x24,                     // mov %edx,(%rsp)
    0x48,0x89,0x7C,0x24,0x08,           // mov %rdi,8(%rsp)
    0x4C,0x8B,0x7C,0x24,0x08,           // mov 8(%rsp),%r15
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0x4C,0x03,0xF8,                     // add %rax,%r15
    0x8B,0xC6,                          // mov %esi,%eax
    0xF0,0x41,0x0F,0xB1,0x0F,           // lock cmpxchg %ecx,(%r15)
    0x8B,0xC8,                          // mov %eax,%ecx
    0x48,0x83,0xC4,0x18,                // add $24,%rsp
    0x41,0x5F,                          // pop %r15
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 28\n"
        "[info] r2 no longer used, setting death = birth\n"
        "[info] Extending r2 live range to 29\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
