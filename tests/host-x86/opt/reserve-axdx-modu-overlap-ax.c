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
static const unsigned int host_opt = BINREC_OPT_H_X86_FIXED_REGS;

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    uint32_t reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocates reg4 = EDX, reg1 = EAX. */
    EXPECT(rtl_add_insn(unit, RTLOP_MODU, reg4, reg1, reg2, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocates reg5 = EDX, but EAX is still live so reg2 doesn't get
     * allocated. */
    EXPECT(rtl_add_insn(unit, RTLOP_MODU, reg5, reg2, reg3, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC0,                          // xor %eax,%eax
    0x33,0xF6,                          // xor %esi,%esi
    0x33,0xFF,                          // xor %edi,%edi
    0x4C,0x8B,0xC0,                     // mov %rax,%r8
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF6,                          // div %esi
    0x49,0x8B,0xC0,                     // mov %r8,%rax
    0x4C,0x8B,0xC0,                     // mov %rax,%r8
    0x8B,0xC6,                          // mov %esi,%eax
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF7,                          // div %edi
    0x49,0x8B,0xC0,                     // mov %r8,%rax
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
