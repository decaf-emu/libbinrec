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

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocates reg3 = EDX, reg1 = EAX (reg2 left unallocated). */
    EXPECT(rtl_add_insn(unit, RTLOP_MULHU, reg3, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocates reg4 = EAX (reg2 doesn't get it due to live range collision),
     * but leaves reg5 unallocated since EDX is still live. */
    EXPECT(rtl_add_insn(unit, RTLOP_MULHU, reg5, reg2, reg4, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, reg4, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg6, 0, 0, 6));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocates reg7 = EDX (reg5 shouldn't block it since it lost EDX to
     * reg3) and reg6 = EAX. */
    EXPECT(rtl_add_insn(unit, RTLOP_MULHU, reg7, reg6, reg6, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg5, reg6, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xBE,0x02,0x00,0x00,0x00,           // mov $2,%esi
    0x48,0x8B,0xF8,                     // mov %rax,%rdi
    0xF7,0xE6,                          // mul %esi
    0x48,0x8B,0xC7,                     // mov %rdi,%rax
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0x48,0x8B,0xFA,                     // mov %rdx,%rdi
    0x4C,0x8B,0xC0,                     // mov %rax,%r8
    0xF7,0xE6,                          // mul %esi
    0x48,0x87,0xD7,                     // xchg %rdx,%rdi
    0x49,0x8B,0xC0,                     // mov %r8,%rax
    0xB8,0x06,0x00,0x00,0x00,           // mov $6,%eax
    0x48,0x8B,0xF0,                     // mov %rax,%rsi
    0xF7,0xE0,                          // mul %eax
    0x48,0x8B,0xC6,                     // mov %rsi,%rax
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
