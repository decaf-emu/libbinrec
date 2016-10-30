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
    int dummy_alias[8];
    for (int i = 0; i < lenof(dummy_alias); i++) {
        EXPECT(dummy_alias[i] = rtl_alloc_alias_register(unit,
                                                         RTLTYPE_V2_FLOAT64));
    }

    alloc_dummy_registers(unit, 8, RTLTYPE_FLOAT32);
    int dummy_regs[13];
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(dummy_regs[i] = rtl_alloc_register(unit, RTLTYPE_INT32));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, dummy_regs[i], 0, 0, 0));
    }

    int reg1, reg2, reg3;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FZCAST, reg3, reg1, 0, 0));
    for (int i = 0; i < lenof(dummy_regs); i++) {
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, dummy_regs[i], 0, 0));
    }
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, reg2, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x53,                               // push %rbx
    0x55,                               // push %rbp
    0x41,0x54,                          // push %r12
    0x41,0x55,                          // push %r13
    0x41,0x56,                          // push %r14
    0x41,0x57,                          // push %r15
    0x48,0x81,0xEC,0x98,0x00,0x00,0x00, // sub $152,%rsp
    0x41,0xBE,0x01,0x00,0x00,0x00,      // mov $1,%r14d
    0x4C,0x89,0xB4,0x24,0x88,0x00,0x00, // mov %r14,136(%rsp)
      0x00,
    0x41,0xBE,0x02,0x00,0x00,0x00,      // mov $2,%r14d
    0x4C,0x8B,0xBC,0x24,0x88,0x00,0x00, // mov 136(%rsp),%r15
      0x00,
    0x4D,0x85,0xFF,                     // test %r15,%r15
    0x78,0x07,                          // js L1
    0xF3,0x4D,0x0F,0x2A,0xC7,           // cvtsi2ss %r15,%xmm8
    0xEB,0x35,                          // jmp L4
    0x0F,0xAE,0x9C,0x24,0x80,0x00,0x00, // L1: stmxcsr 128(%rsp)
      0x00,
    0x49,0xD1,0xEF,                     // shr %r15
    0x0F,0xBA,0xA4,0x24,0x80,0x00,0x00, // btl $13,128(%rsp)
      0x00,0x0D,
    0x73,0x15,                          // jnc L3
    0x0F,0xBA,0xA4,0x24,0x80,0x00,0x00, // btl $14,128(%rsp)
      0x00,0x0E,
    0x73,0x06,                          // jnc L2
    0x41,0xF6,0xC7,0x01,                // test $1,%r15b
    0x74,0x04,                          // jz L3
    0x49,0x83,0xC7,0x01,                // L2: add $1,%r15
    0xF3,0x4D,0x0F,0x2A,0xC7,           // L3: cvtsi2ss %r15,%xmm8
    0xF3,0x45,0x0F,0x58,0xC0,           // addss %xmm8,%xmm8
    0x48,0x81,0xC4,0x98,0x00,0x00,0x00, // L4: add $152,%rsp
    0x41,0x5F,                          // pop %r15
    0x41,0x5E,                          // pop %r14
    0x41,0x5D,                          // pop %r13
    0x41,0x5C,                          // pop %r12
    0x5D,                               // pop %rbp
    0x5B,                               // pop %rbx
    0xC3,                               // ret
};

static const char expected_log[] = "";


/* Tweaked version of tests/rtl-translate-test.i to trigger buffer resize. */

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
        printf("%s:%d: add_rtl(unit) failed (%s)\n%s", __FILE__, line,
               log_messages ? "log follows" : "no errors logged",
               log_messages ? log_messages : "");
        return EXIT_FAILURE;
    }

    EXPECT(rtl_finalize_unit(unit));

    handle->code_buffer_size = 108;
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
