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
    int reg1, reg2, reg3, alias, label1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));  // Gets EAX.
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));  // Gets ECX.
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg2, 0, label1));

    int reg4, reg5, label2;
    /* Allocate ECX (live through the end of the unit) to prevent merging
     * to the same register. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg4, 0, 1));
    /* Don't fall through so that the GET_ALIAS below can be merged without
     * loading. */
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));

    int reg6;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg5 should be merged with reg3 via EAX.  Since EAX dies at the
     * conditional branch above, the merge should not trigger an alias
     * conflict. */
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg6, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg6, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg5, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0x89,0x8F,0x34,0x12,0x00,0x00,      // mov %ecx,0x1234(%rdi)
    0x85,0xC0,                          // test %eax,%eax
    0x8B,0xC1,                          // mov %ecx,%eax
    0x0F,0x84,0x16,0x00,0x00,0x00,      // jz L1
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0xB9,0x05,0x00,0x00,0x00,           // mov $5,%ecx
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nopl 1(%rip)
    0xE9,0x06,0x00,0x00,0x00,           // jmp L2
    0x89,0x87,0x34,0x12,0x00,0x00,      // L1: mov %eax,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // L2: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
