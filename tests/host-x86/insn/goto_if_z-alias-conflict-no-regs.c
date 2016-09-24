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
    int reg1, reg2, reg3, alias, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));  // Gets EAX.
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));  // Gets ECX.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg3, 0, label));

    /* Force EAX to be live past the conditional branch. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 1));

    int reg4, tempreg[13];
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    /* Flood the register table so there are no registers available for
     * alias merging. */
    for (int i = 0; i < lenof(tempreg); i++) {
        EXPECT(tempreg[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, tempreg[i], 0, 0, 0));
    }
    for (int i = 0; i < lenof(tempreg); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, tempreg[i], 0, 0));
    }
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Since reg4 can't be merged, its host_merge field will be left at 0
     * (X86_AX).  That shouldn't trigger an alias conflict just because EAX
     * is live past the branch, since the merge_alias flag will be false. */
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg4, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg4, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0x89,0x8F,0x34,0x12,0x00,0x00,      // mov %ecx,0x1234(%rdi)
    0x85,0xC9,                          // test %ecx,%ecx
    /* If the conflict check goes wrong, this will be turned into a jnz. */
    0x0F,0x84,0x07,0x00,0x00,0x00,      // jz L1
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nop 1(%rip)
    0x33,0xC0,                          // L1: xor %eax,%eax
    0x33,0xC9,                          // xor %ecx,%ecx
    0x33,0xD2,                          // xor %edx,%edx
    0x33,0xF6,                          // xor %esi,%esi
    0x45,0x33,0xC0,                     // xor %r8d,%r8d
    0x45,0x33,0xC9,                     // xor %r9d,%r9d
    0x45,0x33,0xD2,                     // xor %r10d,%r10d
    0x45,0x33,0xDB,                     // xor %r11d,%r11d
    0x33,0xDB,                          // xor %ebx,%ebx
    0x33,0xED,                          // xor %ebp,%ebp
    0x45,0x33,0xE4,                     // xor %r12d,%r12d
    0x45,0x33,0xED,                     // xor %r13d,%r13d
    0x45,0x33,0xF6,                     // xor %r14d,%r14d
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
