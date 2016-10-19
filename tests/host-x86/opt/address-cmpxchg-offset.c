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
static const unsigned int host_opt = BINREC_OPT_H_X86_ADDRESS_OPERANDS;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg3, 0, 0, 3));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg4, reg1, 0, 32));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CMPXCHG, reg5, reg4, reg2, reg3));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x41,0x57,                          // push %r15
    0x8B,0xC2,                          // mov %edx,%eax
    0xF0,0x0F,0xB1,0x4F,0x20,           // lock cmpxchg %ecx,32(%rdi)
    0x8B,0xF0,                          // mov %eax,%esi
    0x41,0x5F,                          // pop %r15
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 3\n"
        "[info] r1 no longer used, setting death = birth\n"
        "[info] Extending r1 live range to 4\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
