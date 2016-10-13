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
    int spill, regs[13], label;
    EXPECT(spill = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, spill, 0, 0, 99));
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, regs[i], 0, 0, i+1));
    }

    int spiller;
    EXPECT(spiller = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, spiller, 0, 0, 98));

    EXPECT(label = rtl_alloc_label(unit));
    rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label);

    int sum;
    rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label);
    EXPECT(sum = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, sum, 0, 0, 0));
    for (int i = 0; i < lenof(regs); i++) {
        int last_sum = sum;
        EXPECT(sum = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_ADD, sum, last_sum, regs[i], 0));
    }
    int last_sum = sum;
    EXPECT(sum = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, sum, last_sum, spill, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xB8,0x63,0x00,0x00,0x00,           // mov $99,%eax
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0xBA,0x02,0x00,0x00,0x00,           // mov $2,%edx
    0xBE,0x03,0x00,0x00,0x00,           // mov $3,%esi
    0xBF,0x04,0x00,0x00,0x00,           // mov $4,%edi
    0x41,0xB8,0x05,0x00,0x00,0x00,      // mov $5,%r8d
    0x41,0xB9,0x06,0x00,0x00,0x00,      // mov $6,%r9d
    0x41,0xBA,0x07,0x00,0x00,0x00,      // mov $7,%r10d
    0x41,0xBB,0x08,0x00,0x00,0x00,      // mov $8,%r11d
    0xBB,0x09,0x00,0x00,0x00,           // mov $9,%ebx
    0xBD,0x0A,0x00,0x00,0x00,           // mov $10,%ebp
    0x41,0xBC,0x0B,0x00,0x00,0x00,      // mov $11,%r12d
    0x41,0xBD,0x0C,0x00,0x00,0x00,      // mov $12,%r13d
    0x41,0xBE,0x0D,0x00,0x00,0x00,      // mov $13,%r14d
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0xB8,0x62,0x00,0x00,0x00,           // mov $98,%eax
    /* EAX has already been spilled here, so it should not be spilled again
     * at the jump. */
    0xE9,0x00,0x00,0x00,0x00,           // jmp L1
    0x33,0xC0,                          // L1: xor %eax,%eax
    0x03,0xC1,                          // add %ecx,%eax
    0x03,0xC2,                          // add %edx,%eax
    0x03,0xC6,                          // add %esi,%eax
    0x03,0xC7,                          // add %edi,%eax
    0x41,0x03,0xC0,                     // add %r8d,%eax
    0x41,0x03,0xC1,                     // add %r9d,%eax
    0x41,0x03,0xC2,                     // add %r10d,%eax
    0x41,0x03,0xC3,                     // add %r11d,%eax
    0x03,0xC3,                          // add %ebx,%eax
    0x03,0xC5,                          // add %ebp,%eax
    0x41,0x03,0xC4,                     // add %r12d,%eax
    0x41,0x03,0xC5,                     // add %r13d,%eax
    0x41,0x03,0xC6,                     // add %r14d,%eax
    0x03,0x04,0x24,                     // add (%rsp),%eax
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
