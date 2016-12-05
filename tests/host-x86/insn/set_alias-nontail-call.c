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
    int reg1, reg2, alias;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, 0, 0));

    int label, reg3, reg4;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg4, reg3, 0, 4));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg4, 0, alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBB,0x01,0x00,0x00,0x00,           // mov $1,%ebx
    /* The use of EAX here is suboptimal; this is because the register
     * allocator sees reg2 dying on the CALL instruction (due to live
     * range extension from aliases) and assumes the register is consumed
     * by the CALL.  This only occurs if a nontail CALL is the last
     * instruction in a block, which currently won't occur during actual
     * translation, so we don't worry about it. */
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x89,0x83,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rbx)
    0x48,0x89,0x04,0x24,                // mov %rax,(%rsp)
    0x0F,0xAE,0x5C,0x24,0x08,           // stmxcsr 8(%rsp)
    0xFF,0xD3,                          // call *%rbx
    0x0F,0xAE,0x54,0x24,0x08,           // ldmxcsr 8(%rsp)
    0x48,0x8B,0x04,0x24,                // mov (%rsp),%rax
    0x83,0xC0,0x04,                     // add $4,%eax
    0x89,0x83,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rbx)
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
