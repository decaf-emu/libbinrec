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
    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    rtl_make_unfoldable(unit, reg1);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, reg2, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBB,0x01,0x00,0x00,0x00,           // mov $1,%ebx
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0xFF,0xD3,                          // call *%rbx
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x48,0x8B,0xC3,                     // mov %rbx,%rax
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
