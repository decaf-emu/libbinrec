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
    .host = BINREC_ARCH_X86_64_WINDOWS,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    uint32_t regs[15];

    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, regs[i], 0, 0, i+1));
    }
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i], 0, 0));
    }

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x56,                               // push %rsi
    0x57,                               // push %rdi
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0xBA,0x03,0x00,0x00,0x00,           // mov $3,%edx
    0x41,0xB8,0x04,0x00,0x00,0x00,      // mov $4,%r8d
    0x41,0xB9,0x05,0x00,0x00,0x00,      // mov $5,%r9d
    0x41,0xBA,0x06,0x00,0x00,0x00,      // mov $6,%r10d
    0x41,0xBB,0x07,0x00,0x00,0x00,      // mov $7,%r11d
    0xBB,0x08,0x00,0x00,0x00,           // mov $8,%ebx
    0xBD,0x09,0x00,0x00,0x00,           // mov $9,%ebp
    0xBE,0x0A,0x00,0x00,0x00,           // mov $10,%esi
    0xBF,0x0B,0x00,0x00,0x00,           // mov $11,%edi
    0x41,0xBC,0x0C,0x00,0x00,0x00,      // mov $12,%r12d
    0x41,0xBD,0x0D,0x00,0x00,0x00,      // mov $13,%r13d
    0x41,0xBE,0x0E,0x00,0x00,0x00,      // mov $14,%r14d
    0x41,0xBF,0x0F,0x00,0x00,0x00,      // mov $15,%r15d
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0x41,0x5F,                          // pop %r15
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5F,                               // pop %rdi
    0x5E,                               // pop %rsi
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
