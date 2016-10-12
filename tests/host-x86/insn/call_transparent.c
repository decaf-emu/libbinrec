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
    alloc_dummy_registers(unit, RTLTYPE_INT32, 1);

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    /* reg1 should not be put in a callee-saved register since the call is
     * a transparent one. */
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL_TRANSPARENT, 0, reg1, 0, 0));
    /* This RETURN should not trigger tail call translation since the call
     * is a transparent one. */
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x18,                // sub $24,%rsp
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0x48,0x89,0x04,0x24,                // mov %rax,(%rsp)
    0x48,0x89,0x4C,0x24,0x08,           // mov %rcx,8(%rsp)
    0xFF,0xD1,                          // call *%rcx
    0x48,0x8B,0x04,0x24,                // mov (%rsp),%rax
    0x48,0x8B,0x4C,0x24,0x08,           // mov 8(%rsp),%rcx
    0xE9,0x00,0x00,0x00,0x00,           // jmp epilogue
    0x48,0x83,0xC4,0x18,                // epilogue: add $24,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
