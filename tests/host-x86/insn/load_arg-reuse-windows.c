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

static int add_rtl(RTLUnit *unit)
{
    uint32_t regs[4][2];
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i][0] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, regs[i][0], 0, 0, i));
    }
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(regs[i][1] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_MOVE, regs[i][1], regs[i][0], 0, 0));
    }
    for (int i = 0; i < lenof(regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i][0], 0, 0));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, regs[i][1], 0, 0));
    }

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x8B,0xC1,                          // mov %ecx,%eax
    0x44,0x8B,0xD2,                     // mov %edx,%r10d
    0x45,0x8B,0xD8,                     // mov %r8d,%r11d
    0x41,0x8B,0xD9,                     // mov %r9d,%ebx
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
