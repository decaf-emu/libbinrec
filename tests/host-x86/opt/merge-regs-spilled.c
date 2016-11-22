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
    int reg1, reg2, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    /* Spill reg2, which should prevent its merging via MERGE_REGS. */
    int spillers[13];
    for (int i = 0; i < lenof(spillers); i++) {
        EXPECT(spillers[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, spillers[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(spillers); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, spillers[i], 0, 0));
    }

    int reg3, reg4, reg5, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    /* reg3 should not avoid EAX. */
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    /* The temporary for loading reg4 should not avoid EAX. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0x40800000));
    /* The temporary for storing reg4 should not avoid EAX. */
    EXPECT(rtl_add_insn(unit, RTLOP_STORE_BR, 0, reg1, reg4, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* This GET_ALIAS should reload the value from storage. */
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg5, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg5, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x8B,0x0C,0x24,                     // mov (%rsp),%ecx
    0xB8,0x03,0x00,0x00,0x00,           // mov $3,%eax
    0xB8,0x00,0x00,0x80,0x40,           // mov $0x40800000,%eax
    0x66,0x0F,0x6E,0xC0,                // movd %eax,%xmm0
    0x66,0x0F,0x7E,0xC0,                // movd %xmm0,%eax
    0x0F,0x38,0xF1,0x07,                // movbe %eax,(%rdi)
    0x89,0x8F,0x34,0x12,0x00,0x00,      // mov %ecx,0x1234(%rdi)
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
