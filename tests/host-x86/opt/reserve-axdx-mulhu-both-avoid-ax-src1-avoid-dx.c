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
    /* Block reg3 from getting EAX. */
    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CMPXCHG, reg3, reg1, reg2, reg2));

    /* Block reg5 from getting EAX or EDX. */
    int reg4, reg5, reg6;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_DIVU, reg6, reg4, reg5, 0));

    int reg7;
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg3 should get EDX here, with EAX remaining unused.  The assignment
     * of EDX is actually suboptimal in this particular case since it
     * forces reg5 to get saved and restored around the intervening divide
     * instruction, but this is a contrived case which is probably unlikely
     * to occur in real-world code, so we don't currently worry about it. */
    EXPECT(rtl_add_insn(unit, RTLOP_MULHU, reg7, reg5, reg3, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x41,0x57,                          // push %r15
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x8B,0xD0,                          // mov %eax,%edx
    0xF0,0x0F,0xB1,0x11,                // lock cmpxchg %edx,(%rcx)
    0x8B,0xD0,                          // mov %eax,%edx
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0xB9,0x05,0x00,0x00,0x00,           // mov $5,%ecx
    0x48,0x8B,0xF2,                     // mov %rdx,%rsi
    0x33,0xD2,                          // xor %edx,%edx
    0xF7,0xF1,                          // div %ecx
    0x48,0x8B,0xD6,                     // mov %rsi,%rdx
    0x8B,0xC1,                          // mov %ecx,%eax
    0xF7,0xE2,                          // mul %edx
    0x41,0x5F,                          // pop %r15
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
