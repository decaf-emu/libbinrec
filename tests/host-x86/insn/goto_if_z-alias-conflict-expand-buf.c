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
#include "tests/mem-wrappers.h"


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_SYSV,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    uint32_t reg1, reg2, reg3, alias, label1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias, reg1, 0x1234);
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0));  // Gets EAX.
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 0));  // Gets ECX.
    /* Add a bunch of NOPs to pad the code past the length reserved for
     * the prologue. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 4));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 5));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 6));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 7));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 8));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 9));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 10));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 11));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 12));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg3, 0, label1));

    uint32_t reg4, label2;
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* Allocate ECX (live through the end of the unit) to prevent merging
     * to the same register. */
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg4, 0, 0, 0));
    /* Force EAX to be live past the conditional branch. */
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg2, 0, 1));
    /* Don't fall through so that the GET_ALIAS below can be merged without
     * loading. */
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));

    /* This block is unreachable, but we set up a bunch of aliases here
     * which are merged into the next block in order to pad the alias setup
     * code to the point where it will exceed the available space in the
     * output buffer.  (We need to pad the setup code because the
     * translator always reserves a fixed size, currently 28 bytes (see
     * MAX_INSN_LEN), for all instructions.) */
    uint32_t pad_alias[5], pad_reg[5];
    STATIC_ASSERT(lenof(pad_alias) == lenof(pad_reg), "Array length mismatch");
    for (int i = 0; i < lenof(pad_alias); i++) {
        EXPECT(pad_alias[i] = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
        rtl_set_alias_storage(unit, pad_alias[i], reg1, 0x100+i*4);
        EXPECT(pad_reg[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, pad_reg[i], 0, 0, 0));
        EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS,
                            0, pad_reg[i], 0, pad_alias[i]));
    }

    uint32_t reg5;
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    /* reg5 should be merged with reg3 via EAX.  Since EAX is live past the
     * conditional branch above, the merge should trigger an alias conflict
     * which will flip the sense of the conditional branch so the merge can
     * be performed conditionally. */
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg5, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg5, 0, alias));
    /* Also reload the padding aliases to trigger merging. */
    for (int i = 0; i < lenof(pad_alias); i++) {
        uint32_t reg;
        EXPECT(reg = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg, 0, 0, pad_alias[i]));
    }

    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg4, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0x33,0xC0,                          // xor %eax,%eax
    0x33,0xC9,                          // xor %ecx,%ecx
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nop 1(%rip)
    0x0F,0x1F,0x05,0x02,0x00,0x00,0x00, // nop 2(%rip)
    0x0F,0x1F,0x05,0x03,0x00,0x00,0x00, // nop 3(%rip)
    0x0F,0x1F,0x05,0x04,0x00,0x00,0x00, // nop 4(%rip)
    0x0F,0x1F,0x05,0x05,0x00,0x00,0x00, // nop 5(%rip)
    0x0F,0x1F,0x05,0x06,0x00,0x00,0x00, // nop 6(%rip)
    0x0F,0x1F,0x05,0x07,0x00,0x00,0x00, // nop 7(%rip)
    0x0F,0x1F,0x05,0x08,0x00,0x00,0x00, // nop 8(%rip)
    0x0F,0x1F,0x05,0x09,0x00,0x00,0x00, // nop 9(%rip)
    0x0F,0x1F,0x05,0x0A,0x00,0x00,0x00, // nop 10(%rip)
    0x0F,0x1F,0x05,0x0B,0x00,0x00,0x00, // nop 11(%rip)
    0x0F,0x1F,0x05,0x0C,0x00,0x00,0x00, // nop 12(%rip)
    0x89,0x8F,0x34,0x12,0x00,0x00,      // mov %ecx,0x1234(%rdi)
    0x85,0xC9,                          // test %ecx,%ecx
    0x75,0x28,                          // jnz L0
    0x8B,0xC1,                          // mov %ecx,%eax
    0x8B,0x97,0x00,0x01,0x00,0x00,      // mov 0x100(%rdi),%edx
    0x8B,0xB7,0x04,0x01,0x00,0x00,      // mov 0x104(%rdi),%esi
    0x44,0x8B,0x87,0x08,0x01,0x00,0x00, // mov 0x108(%rdi),%r8d
    0x44,0x8B,0x8F,0x0C,0x01,0x00,0x00, // mov 0x10C(%rdi),%r9d
    0x44,0x8B,0x97,0x10,0x01,0x00,0x00, // mov 0x110(%rdi),%r10d
    0xE9,0x4F,0x00,0x00,0x00,           // jmp L1
    0x33,0xC9,                          // L0: xor %ecx,%ecx
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nop 1(%rip)
    0xE9,0x47,0x00,0x00,0x00,           // jmp L2
    0x33,0xC0,                          // xor %eax,%eax
    0x89,0x87,0x00,0x01,0x00,0x00,      // mov %eax,0x100(%rdi)
    0x33,0xD2,                          // xor %edx,%edx
    0x89,0x97,0x04,0x01,0x00,0x00,      // mov %edx,0x104(%rdi)
    0x33,0xF6,                          // xor %esi,%esi
    0x89,0xB7,0x08,0x01,0x00,0x00,      // mov %esi,0x108(%rdi)
    0x45,0x33,0xC0,                     // xor %r8d,%r8d
    0x44,0x89,0x87,0x0C,0x01,0x00,0x00, // mov %r8d,0x10C(%rdi)
    0x45,0x33,0xC9,                     // xor %r9d,%r9d
    0x44,0x89,0x8F,0x10,0x01,0x00,0x00, // mov %r9d,0x110(%rdi)
    0x48,0x87,0xC2,                     // xchg %rax,%rdx
    0x48,0x87,0xC6,                     // xchg %rax,%rsi
    0x49,0x87,0xC0,                     // xchg %rax,%r8
    0x49,0x87,0xC1,                     // xchg %rax,%r9
    0x44,0x8B,0xD0,                     // mov %eax,%r10d
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // L1: mov %eax,0x1234(%rdi)
    0x48,0x83,0xC4,0x08,                // L2: add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";


/* Tweaked version of tests/rtl-translate-test.i to trigger buffer resize. */

#include "src/memory.h"
#include "tests/log-capture.h"

int main(void)
{
    binrec_setup_t final_setup = setup;
    final_setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));
    binrec_set_optimization_flags(handle, 0, 0, host_opt);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (add_rtl(unit) != EXIT_SUCCESS) {
        const int line = __LINE__ - 1;
        const char *log_messages = get_log_messages();
        fprintf(stderr, "%s:%d: add_rtl(unit) failed (%s)\n%s", __FILE__, line,
                log_messages ? "log follows" : "no errors logged",
                log_messages ? log_messages : "");
        return EXIT_FAILURE;
    }

    EXPECT(rtl_finalize_unit(unit));

    /* We have to set the buffer to a precise size to trigger the
     * buffer-full logic in alias conflict resolution. */
    handle->code_buffer_size = 128;
    handle->code_alignment = 16;
    EXPECT(handle->code_buffer = binrec_code_malloc(
               handle, handle->code_buffer_size, handle->code_alignment));

    if (sizeof(expected_code) > 0) {
        EXPECT(host_x86_translate(handle, unit));
        if (!(handle->code_len == sizeof(expected_code)
              && memcmp(handle->code_buffer, expected_code,
                        sizeof(expected_code)) == 0)) {
            const char *log_messages = get_log_messages();
            if (log_messages) {
                fputs(log_messages, stdout);
            }
            EXPECT_MEMEQ(handle->code_buffer, expected_code,
                         sizeof(expected_code));
            EXPECT_EQ(handle->code_len, sizeof(expected_code));
        }
    } else {
        EXPECT_FALSE(host_x86_translate(handle, unit));
    }

    const char *log_messages = get_log_messages();
    EXPECT_STREQ(log_messages, *expected_log ? expected_log : NULL);

    binrec_code_free(handle, handle->code_buffer);
    handle->code_buffer = NULL;
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
