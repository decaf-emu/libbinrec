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
    alloc_dummy_registers(unit, 5, RTLTYPE_INT32);
    alloc_dummy_registers(unit, 1, RTLTYPE_FLOAT);

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x20,                // sub $32,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x48,0x89,0x04,0x24,                // mov %rax,(%rsp)
    /* The XMM0 slot should be properly aligned. */
    0x0F,0x29,0x44,0x24,0x10,           // movaps %xmm0,16(%rsp)
    0xFF,0xD0,                          // call *%rax
    0x48,0x8B,0x04,0x24,                // mov (%rsp),%rax
    0x0F,0x28,0x44,0x24,0x10,           // movaps 16(%rsp),%xmm0
    /* The second CALL should reuse the same frame slots as the first. */
    0x48,0x89,0x04,0x24,                // mov %rax,(%rsp)
    0x0F,0x29,0x44,0x24,0x10,           // movaps %xmm0,16(%rsp)
    0xFF,0xD0,                          // call *%rax
    0x48,0x8B,0x04,0x24,                // mov (%rsp),%rax
    0x0F,0x28,0x44,0x24,0x10,           // movaps 16(%rsp),%xmm0
    0x48,0x83,0xC4,0x20,                // add $32,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
