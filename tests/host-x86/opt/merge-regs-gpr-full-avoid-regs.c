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
    int alias[12], merge_src[12], reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 101));
    for (int i = 0; i < lenof(alias); i++) {
        EXPECT(alias[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
        EXPECT(merge_src[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, merge_src[i], 0, 0, i+1));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, merge_src[i], 0, alias[i]));
    }
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 102));

    int reg3, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    /* The register allocator should cancel one of the merges to make
     * room for this register, but not ECX since that's prohibited for
     * shift instructions. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLL, reg3, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
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
    0x48,0x83,0xEC,0x30,                // sub $48,%rsp
    0xB8,0x65,0x00,0x00,0x00,           // mov $101,%eax
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0x89,0x0C,0x24,                     // mov %ecx,(%rsp)
    0xBA,0x02,0x00,0x00,0x00,           // mov $2,%edx
    0x89,0x54,0x24,0x04,                // mov %edx,4(%rsp)
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
    0x41,0xBE,0x66,0x00,0x00,0x00,      // mov $102,%r14d
    0x8B,0xD0,                          // mov %eax,%edx
    0x41,0x8B,0xCE,                     // mov %r14d,%ecx
    0xD3,0xE2,                          // shl %cl,%edx
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x8B,0x44,0x24,0x04,                // mov 4(%rsp),%eax
    0x89,0x44,0x24,0x04,                // mov %eax,4(%rsp)
    0x89,0x74,0x24,0x08,                // mov %esi,8(%rsp)
    0x89,0x7C,0x24,0x0C,                // mov %edi,12(%rsp)
    0x44,0x89,0x44,0x24,0x10,           // mov %r8d,16(%rsp)
    0x44,0x89,0x4C,0x24,0x14,           // mov %r9d,20(%rsp)
    0x44,0x89,0x54,0x24,0x18,           // mov %r10d,24(%rsp)
    0x44,0x89,0x5C,0x24,0x1C,           // mov %r11d,28(%rsp)
    0x89,0x5C,0x24,0x20,                // mov %ebx,32(%rsp)
    0x89,0x6C,0x24,0x24,                // mov %ebp,36(%rsp)
    0x44,0x89,0x64,0x24,0x28,           // mov %r12d,40(%rsp)
    0x44,0x89,0x6C,0x24,0x2C,           // mov %r13d,44(%rsp)
    0x48,0x83,0xC4,0x30,                // add $48,%rsp
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
