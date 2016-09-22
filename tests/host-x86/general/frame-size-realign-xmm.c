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
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    alloc_dummy_registers(unit, 7, RTLTYPE_FLOAT);

    uint32_t reg, alias;
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, alias));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    /* The function's frame size is 4 bytes (for the single alias).  This
     * would fit within the post-PUSH 8-byte stack realignment, but we also
     * have to save an XMM register, so we can't reuse that 8-byte region. */
    0x48,0x83,0xEC,0x28,                // sub $40,%rsp
    0x0F,0x29,0x74,0x24,0x10,           // movaps %xmm6,16(%rsp)
    0x8B,0x04,0x24,                     // mov (%rsp),%eax
    0x0F,0x28,0x74,0x24,0x10,           // movaps 16(%rsp),%xmm6
    0x48,0x83,0xC4,0x28,                // add $40,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
