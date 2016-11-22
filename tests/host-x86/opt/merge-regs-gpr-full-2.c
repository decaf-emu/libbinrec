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
    /* Test additional code paths for merge cancelling. */

    /* Having an alias at the beginning of the alias list which is not
     * loaded by the next block should not confuse the register allocator
     * into trying to cancel a merge on it. */
    int unloaded_alias, reg1;
    EXPECT(unloaded_alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, unloaded_alias));

    /* An alias first loaded in the next block should also not cause
     * problems. */
    int unstored_alias;
    EXPECT(unstored_alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));

    /* Set the aliases in reverse order so the allocator cancels the last
     * one in the list (since it has the numerically first host register). */
    int alias[14], merge_src[14];
    for (int i = 0; i < lenof(alias); i++) {
        EXPECT(alias[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    }
    for (int i = lenof(alias)-1; i >= 0; i--) {
        EXPECT(merge_src[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, merge_src[i], 0, 0, i+1));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, merge_src[i], 0, alias[i]));
    }

    int reg2, reg3, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    /* The register allocator should cancel one of the merges to make
     * room for this register. */
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 101));
    for (int i = 0; i < lenof(alias); i++) {
        int merge_dest;
        EXPECT(merge_dest = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS,
                            merge_dest, 0, 0, alias[i]));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, merge_dest, 0, alias[i]));
    }
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, unstored_alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, unstored_alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x40,                // sub $64,%rsp
    0x33,0xC0,                          // xor %eax,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0xB8,0x0E,0x00,0x00,0x00,           // mov $14,%eax
    0x89,0x44,0x24,0x3C,                // mov %eax,60(%rsp)
    0xB9,0x0D,0x00,0x00,0x00,           // mov $13,%ecx
    0xBA,0x0C,0x00,0x00,0x00,           // mov $12,%edx
    0xBE,0x0B,0x00,0x00,0x00,           // mov $11,%esi
    0xBF,0x0A,0x00,0x00,0x00,           // mov $10,%edi
    0x41,0xB8,0x09,0x00,0x00,0x00,      // mov $9,%r8d
    0x41,0xB9,0x08,0x00,0x00,0x00,      // mov $8,%r9d
    0x41,0xBA,0x07,0x00,0x00,0x00,      // mov $7,%r10d
    0x41,0xBB,0x06,0x00,0x00,0x00,      // mov $6,%r11d
    0xBB,0x05,0x00,0x00,0x00,           // mov $5,%ebx
    0xBD,0x04,0x00,0x00,0x00,           // mov $4,%ebp
    0x41,0xBC,0x03,0x00,0x00,0x00,      // mov $3,%r12d
    0x41,0xBD,0x02,0x00,0x00,0x00,      // mov $2,%r13d
    0x41,0xBE,0x01,0x00,0x00,0x00,      // mov $1,%r14d
    0xB8,0x65,0x00,0x00,0x00,           // mov $101,%eax
    0x44,0x89,0x74,0x24,0x08,           // mov %r14d,8(%rsp)
    0x44,0x89,0x6C,0x24,0x0C,           // mov %r13d,12(%rsp)
    0x44,0x89,0x64,0x24,0x10,           // mov %r12d,16(%rsp)
    0x89,0x6C,0x24,0x14,                // mov %ebp,20(%rsp)
    0x89,0x5C,0x24,0x18,                // mov %ebx,24(%rsp)
    0x44,0x89,0x5C,0x24,0x1C,           // mov %r11d,28(%rsp)
    0x44,0x89,0x54,0x24,0x20,           // mov %r10d,32(%rsp)
    0x44,0x89,0x4C,0x24,0x24,           // mov %r9d,36(%rsp)
    0x44,0x89,0x44,0x24,0x28,           // mov %r8d,40(%rsp)
    0x89,0x7C,0x24,0x2C,                // mov %edi,44(%rsp)
    0x89,0x74,0x24,0x30,                // mov %esi,48(%rsp)
    0x89,0x54,0x24,0x34,                // mov %edx,52(%rsp)
    0x89,0x4C,0x24,0x38,                // mov %ecx,56(%rsp)
    0x8B,0x44,0x24,0x3C,                // mov 60(%rsp),%eax
    0x89,0x44,0x24,0x3C,                // mov %eax,60(%rsp)
    0x8B,0x44,0x24,0x04,                // mov 4(%rsp),%eax
    0x89,0x44,0x24,0x04,                // mov %eax,4(%rsp)
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
