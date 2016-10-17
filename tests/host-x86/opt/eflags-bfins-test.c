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
static const unsigned int host_opt = BINREC_OPT_H_X86_CONDITION_CODES;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg4, reg1, reg2, 2 | 4<<8));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg5, reg3, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_BFINS, reg6, reg4, reg4, 2 | 4<<8));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_SEQI, reg7, reg6, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB8,0x01,0x00,0x00,0x00,           // mov $1,%eax
    0xB9,0x02,0x00,0x00,0x00,           // mov $2,%ecx
    0x8B,0xD0,                          // mov %eax,%edx
    0x03,0xD1,                          // add %ecx,%edx
    0x83,0xE0,0xC3,                     // and $0xFFFFFFC3,%eax
    0x83,0xE1,0x0F,                     // and $0xF,%ecx
    0xC1,0xE1,0x02,                     // shl $2,%ecx
    0x0B,0xC1,                          // or %ecx,%eax
    0x85,0xD2,                          // test %edx,%edx
    0x0F,0x94,0xC2,                     // setz %dl
    0x0F,0xB6,0xD2,                     // movzbl %dl,%edx
    0x8B,0xC8,                          // mov %eax,%ecx
    0x83,0xE1,0xC3,                     // and $0xFFFFFFC3,%ecx
    0x83,0xE0,0x0F,                     // and $0xF,%eax
    0xC1,0xE0,0x02,                     // shl $2,%eax
    0x0B,0xC8,                          // or %eax,%ecx
    0x85,0xC9,                          // test %ecx,%ecx
    0x0F,0x94,0xC1,                     // setz %cl
    0x0F,0xB6,0xC9,                     // movzbl %cl,%ecx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
