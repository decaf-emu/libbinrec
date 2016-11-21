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
static const unsigned int host_opt = BINREC_OPT_H_X86_MERGE_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));  // Gets ECX.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 0));

    int reg4, reg5, reg6, label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));  // Gets EAX.
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    /* The temporary for the conversion should not clobber ECX. */
    EXPECT(rtl_add_insn(unit, RTLOP_FZCAST, reg5, reg4, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg6, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg6, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0x48,0x85,0xC0,                     // test %rax,%rax
    0x78,0x07,                          // js L1
    0xF3,0x48,0x0F,0x2A,0xC0,           // cvtsi2ss %rax,%xmm0
    0xEB,0x2A,                          // jmp L4
    0x48,0x8B,0xD0,                     // L1: mov %rax,%rdx
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x48,0xD1,0xEA,                     // shr %rdx
    0x0F,0xBA,0x24,0x24,0x0D,           // btl $13,(%rsp)
    0x73,0x10,                          // jnc L3
    0x0F,0xBA,0x24,0x24,0x0E,           // btl $14,(%rsp)
    0x73,0x05,                          // jnc L2
    0xF6,0xC2,0x01,                     // test $1,%dl
    0x74,0x04,                          // jz L3
    0x48,0x83,0xC2,0x01,                // L2: add $1,%rdx
    0xF3,0x48,0x0F,0x2A,0xC2,           // L3: cvtsi2ss %rdx,%xmm0
    0xF3,0x0F,0x58,0xC0,                // addss %xmm0,%xmm0
    0x89,0x8F,0x34,0x12,0x00,0x00,      // L4: mov %ecx,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
