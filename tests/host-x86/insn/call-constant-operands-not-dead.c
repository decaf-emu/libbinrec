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
    /* These constants should not be optimized out since they are live
     * past the CALL which uses them. */
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
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x48,0x83,0xEC,0x10,                // sub $16,%rsp
    0xBB,0x01,0x00,0x00,0x00,           // mov $1,%ebx
    0xBD,0x02,0x00,0x00,0x00,           // mov $2,%ebp
    0x41,0xBC,0x03,0x00,0x00,0x00,      // mov $3,%r12d
    0xB8,0x04,0x00,0x00,0x00,           // mov $4,%eax
    0xB9,0x05,0x00,0x00,0x00,           // mov $5,%ecx
    0x89,0x08,                          // mov %ecx,(%rax)
    0x8B,0xFD,                          // mov %ebp,%edi
    0x41,0x8B,0xF4,                     // mov %r12d,%esi
    0xFF,0xD3,                          // call *%rbx
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x83,0x24,0x24,0xC0,                // andl $-64,(%rsp)
    0x0F,0xAE,0x14,0x24,                // ldmxcsr (%rsp)
    0x48,0x83,0xC4,0x10,                // add $16,%rsp
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
