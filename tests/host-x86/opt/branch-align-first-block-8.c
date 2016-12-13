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
static const unsigned int host_opt = BINREC_OPT_H_X86_BRANCH_ALIGNMENT;

static int add_rtl(RTLUnit *unit)
{
    int label;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    alloc_dummy_registers(unit, 14, RTLTYPE_INT32);
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x0F,0x1F,0x84,0x00,0x00,0x00,0x00, // nopl 0(%rax,%rax)
      0x00,
    0xEB,0xFE,                          // L1: jmp L1
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
