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
    int reg1, reg2, alias, label;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));  // Gets EAX.
    /* This needs to load the alias to EAX if the branch is taken, but the
     * load should be conditional so as not to clobber reg2. */
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg2, 0, label));

    int reg3;
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg3, reg2, 0, 3));
    /* Everything happens in EAX, so it'll be used as the merge target in
     * the following block. */
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));

    int reg4, reg5;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg4, 0, 0, alias));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg5, reg4, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg5, 0, alias));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x85,0xC0,                          // test %eax,%eax
    /* If the conflict check goes wrong, the reload will occur on the
     * common path here. */
    0x75,0x0B,                          // jnz L0
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0xE9,0x03,0x00,0x00,0x00,           // jmp L1
    0x83,0xC0,0x03,                     // L0: add $3,%eax
    0x83,0xC0,0x05,                     // L1: add $5,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
