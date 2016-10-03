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
    int label, alias;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));

    for (int i = 0; i < 8; i++) {
        int reg;
        EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg, 0, 0, i+1));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg, 0, alias));
        EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg, 0, label));
    }

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 99));

    int reg2, reg3;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg2, 0, 0, alias));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg3, reg2, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x75,0x00,0x00,0x00,      // jz L1
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x65,0x00,0x00,0x00,      // jz L1
    0xB8,0x03,0x00,0x00,0x00,           // mov $3,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x55,0x00,0x00,0x00,      // jz L1
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x45,0x00,0x00,0x00,      // jz L1
    0xB8,0x05,0x00,0x00,0x00,           // mov $5,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x35,0x00,0x00,0x00,      // jz L1
    0xB8,0x06,0x00,0x00,0x00,           // mov $6,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x25,0x00,0x00,0x00,      // jz L1
    0xB8,0x07,0x00,0x00,0x00,           // mov $7,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x15,0x00,0x00,0x00,      // jz L1
    0xB8,0x08,0x00,0x00,0x00,           // mov $8,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x85,0xC0,                          // test %eax,%eax
    0x0F,0x84,0x05,0x00,0x00,0x00,      // jz L1
    /* This should not overwrite EAX. */
    0xB9,0x63,0x00,0x00,0x00,           // mov $99,%ecx
    0x83,0xC0,0x01,                     // L1: add $1,%eax
    0x89,0x04,0x24,                     // mov %eax,(%rsp)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
