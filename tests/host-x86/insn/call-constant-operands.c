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
    int reg1, reg2, reg3, reg4, reg5;
    /* These constants should all be loaded right before the actual call,
     * after the store of reg5. */
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 4));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_STORE, 0, reg4, reg5, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, reg2, reg3));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0xB9,0x05,0x00,0x00,0x00,           // mov $5,%ecx
    0x89,0x08,                          // mov %ecx,(%rax)
    0xBF,0x02,0x00,0x00,0x00,           // mov $2,%edi
    0xBE,0x03,0x00,0x00,0x00,           // mov $3,%esi
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xFF,0xE0,                          // jmp *%rax
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Killing instruction 0\n"
        "[info] Killing instruction 1\n"
        "[info] Killing instruction 2\n"
    #endif
    "";

#include "tests/rtl-translate-test.i"
