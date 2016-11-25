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
    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));

    alloc_dummy_registers(unit, 2, RTLTYPE_INT32);
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    rtl_make_unfoldable(unit, reg3);
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    rtl_make_unfoldable(unit, reg2);
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    rtl_make_unfoldable(unit, reg1);
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, reg2, reg3));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xBA,0x03,0x00,0x00,0x00,           // mov $3,%edx
    0xBE,0x02,0x00,0x00,0x00,           // mov $2,%esi
    0xBF,0x01,0x00,0x00,0x00,           // mov $1,%edi
    0x48,0x8B,0xC7,                     // mov %rdi,%rax
    0x8B,0xFE,                          // mov %esi,%edi
    0x8B,0xF2,                          // mov %edx,%esi
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xFF,0xE0,                          // jmp *%rax
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
