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
    .host = BINREC_ARCH_X86_64_WINDOWS,
};
static const unsigned int host_opt = BINREC_OPT_H_X86_FIXED_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6;
    /* EAX and ECX are taken by FIXED_REGS below, so the first argument
     * needs to be moved to a different register.  The next two free
     * registers are RDX and R8, but if reg1 got RDX, the LOAD_ARG would
     * clobber the second argument, so the allocator should assign R8
     * to reg1 instead (thus leaving the second argument in its initial
     * regsiter). */
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg3, reg1, 0, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg4, reg3, reg3, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg5, reg1, 0, 4));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SLL, reg6, reg4, reg5, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x4C,0x8B,0xC1,                     // mov %rcx,%r8
    0x41,0x8B,0x00,                     // mov (%r8),%eax
    0x4C,0x8B,0xD2,                     // mov %rdx,%r10
    0x4C,0x8B,0xC8,                     // mov %rax,%r9
    0x33,0xD2,                          // xor %edx,%edx
    0x41,0xF7,0xF1,                     // div %r9d
    0x44,0x8B,0xC8,                     // mov %eax,%r9d
    0x49,0x8B,0xD2,                     // mov %r10,%rdx
    0x41,0x8B,0x48,0x04,                // mov 4(%r8),%ecx
    0x41,0xD3,0xE1,                     // shl %cl,%r9d
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
