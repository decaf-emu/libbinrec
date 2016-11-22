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
    .host_features = BINREC_FEATURE_X86_MOVBE,
};
static const unsigned int host_opt = BINREC_OPT_H_X86_MERGE_REGS;

static int add_rtl(RTLUnit *unit)
{
    int alias[14], merge_src[14];
    for (int i = 0; i < lenof(alias); i++) {
        EXPECT(alias[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
        EXPECT(merge_src[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, merge_src[i], 0, 0, i+1));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, merge_src[i], 0, alias[i]));
    }

    int reg1, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    /* The register allocator should cancel one of the merges to make
     * room for this register. */
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 101));
    for (int i = 0; i < lenof(alias); i++) {
        int merge_dest;
        EXPECT(merge_dest = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS,
                            merge_dest, 0, 0, alias[i]));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, merge_dest, 0, alias[i]));
    }

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x40,                // sub $64,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
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
    0xB8,0x65,0x00,0x00,0x00,           // mov $101,%eax
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x89,0x4C,0x24,0x04,                // mov %ecx,4(%rsp)
    0x89,0x54,0x24,0x08,                // mov %edx,8(%rsp)
    0x89,0x74,0x24,0x0C,                // mov %esi,12(%rsp)
    0x89,0x7C,0x24,0x10,                // mov %edi,16(%rsp)
    0x44,0x89,0x44,0x24,0x14,           // mov %r8d,20(%rsp)
    0x44,0x89,0x4C,0x24,0x18,           // mov %r9d,24(%rsp)
    0x44,0x89,0x54,0x24,0x1C,           // mov %r10d,28(%rsp)
    0x44,0x89,0x5C,0x24,0x20,           // mov %r11d,32(%rsp)
    0x89,0x5C,0x24,0x24,                // mov %ebx,36(%rsp)
    0x89,0x6C,0x24,0x28,                // mov %ebp,40(%rsp)
    0x44,0x89,0x64,0x24,0x2C,           // mov %r12d,44(%rsp)
    0x44,0x89,0x6C,0x24,0x30,           // mov %r13d,48(%rsp)
    0x44,0x89,0x74,0x24,0x34,           // mov %r14d,52(%rsp)
    0x48,0x83,0xC4,0x40,                // add $64,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
