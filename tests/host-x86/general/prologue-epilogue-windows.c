/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"
#include "src/host-x86.h"
#include "src/memory.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.host = BINREC_ARCH_X86_64_WINDOWS;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    /* Allocate enough RTL registers to use up all available GPRs. */
    uint8_t reg_gpr[15];
    for (int i = 0; i < lenof(reg_gpr); i++) {
        EXPECT(reg_gpr[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
    }

    /* Insert NOPs and rewrite their destination register fields as in
     * the basic prologue/epilogue test. */
    for (int i = 0; i < lenof(reg_gpr); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
        unit->insns[unit->num_insns-1].dest = reg_gpr[i];
        unit->regs[reg_gpr[i]].birth = i;
        unit->regs[reg_gpr[i]].death = lenof(reg_gpr);
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));  // Registers die here.

    EXPECT(rtl_finalize_unit(unit));

    static const uint8_t expected_code[] = {
        0x53,                           // push %rbx
        0x55,                           // push %rbp
        0x56,                           // push %rsi
        0x57,                           // push %rdi
        0x41,0x54,                      // push %r12
        0x41,0x55,                      // push %r13
        0x41,0x56,                      // push %r14
        0x41,0x57,                      // push %r15
        0x48,0x83,0xEC,0x08,            // sub $8,%rsp  # for stack alignment
        0x48,0x83,0xC4,0x08,            // add $8,%rsp
        0x41,0x5F,                      // pop %r15
        0x41,0x5E,                      // pop %r14
        0x41,0x5D,                      // pop %r13
        0x41,0x5C,                      // pop %r12
        0x5F,                           // pop %rdi
        0x5E,                           // pop %rsi
        0x5D,                           // pop %rbp
        0x5B,                           // pop %rbx
        0xC3,                           // ret
    };

    handle->code_buffer_size = sizeof(expected_code);
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));
    EXPECT(host_x86_translate(handle, unit));
    uint8_t *code = handle->code_buffer;
    long code_len = handle->code_len;
    handle->code_buffer = NULL;

    EXPECT_MEMEQ(code, expected_code, sizeof(expected_code));
    EXPECT_EQ(code_len, sizeof(expected_code));

    binrec_code_free(handle, code);
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
