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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS
                                   | BINREC_OPT_H_X86_FIXED_REGS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6, reg7, reg8, reg9;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    /* reg4 will get RDX here from fixed-regs allocation. */
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MULHU, reg4, reg2, reg3, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg6, 0, 0, 6));
    /* reg4 initially dies here. */
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg7, reg1, reg4, 0));
    /* reg8 should also be allocated RDX during the fixed-regs pass since
     * (at this point) reg4 is dead. */
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_MULHS, reg8, reg5, reg6, 0));
    /* This load will extend the live range of reg4 so that it overlaps
     * reg8, causing reg8 to lose its fixed-regs allocation and get RCX
     * from the default allocation rule (since RAX is avoided). */
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg9, reg7, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xBA,0x02,0x00,0x00,0x00,           // mov $2,%edx
    0xB8,0x03,0x00,0x00,0x00,           // mov $3,%eax
    0x48,0xF7,0xE2,                     // mul %rdx
    0xB8,0x05,0x00,0x00,0x00,           // mov $5,%eax
    0xB9,0x06,0x00,0x00,0x00,           // mov $6,%ecx
    0x48,0x87,0xCA,                     // xchg %rcx,%rdx
    0x48,0xF7,0xEA,                     // imul %rdx
    0x48,0x87,0xD1,                     // xchg %rdx,%rcx
    0x8B,0x3C,0x17,                     // mov (%rdi,%rdx),%edi
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 6\n"
        "[info] r4 no longer used, setting death = birth\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Extending r1 live range to 8\n"
        "[info] Extending r4 live range to 8\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
