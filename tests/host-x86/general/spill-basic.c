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
    uint32_t regs[14], sum;
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, regs[i], 0, 0, i+1));
    }
    EXPECT(sum = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, sum, 0, 0, 0));
    for (int i = 0; i < lenof(regs); i++) {
        uint32_t last_sum = sum;
        EXPECT(sum = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_ADD, sum, last_sum, regs[i], 0));
    }

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
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0xBA,0x03,0x00,0x00,0x00,           // mov $3,%edx
    0xBE,0x04,0x00,0x00,0x00,           // mov $4,%esi
    0xBF,0x05,0x00,0x00,0x00,           // mov $5,%edi
    0x41,0xB8,0x06,0x00,0x00,0x00,      // mov $6,%r8d
    0x41,0xB9,0x07,0x00,0x00,0x00,      // mov $7,%r9d
    0x41,0xBA,0x08,0x00,0x00,0x00,      // mov $8,%r10d
    0x41,0xBB,0x09,0x00,0x00,0x00,      // mov $9,%r11d
    0xBB,0x0A,0x00,0x00,0x00,           // mov $10,%ebx
    0xBD,0x0B,0x00,0x00,0x00,           // mov $11,%ebp
    0x41,0xBC,0x0C,0x00,0x00,0x00,      // mov $12,%r12d
    0x41,0xBD,0x0D,0x00,0x00,0x00,      // mov $13,%r13d
    0x41,0xBE,0x0E,0x00,0x00,0x00,      // mov $14,%r14d
    0x44,0x89,0x34,0x24,                // mov %r14d,(%rsp)
    0x45,0x33,0xF6,                     // xor %r14d,%r14d
    0x44,0x03,0xF0,                     // add %eax,%r14d
    0x44,0x03,0xF1,                     // add %ecx,%r14d
    0x44,0x03,0xF2,                     // add %edx,%r14d
    0x44,0x03,0xF6,                     // add %esi,%r14d
    0x44,0x03,0xF7,                     // add %edi,%r14d
    0x45,0x03,0xF0,                     // add %r8d,%r14d
    0x45,0x03,0xF1,                     // add %r9d,%r14d
    0x45,0x03,0xF2,                     // add %r10d,%r14d
    0x45,0x03,0xF3,                     // add %r11d,%r14d
    0x44,0x03,0xF3,                     // add %ebx,%r14d
    0x44,0x03,0xF5,                     // add %ebp,%r14d
    0x45,0x03,0xF4,                     // add %r12d,%r14d
    0x45,0x03,0xF5,                     // add %r13d,%r14d
    0x44,0x03,0x34,0x24,                // add (%rsp),%r14d
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
