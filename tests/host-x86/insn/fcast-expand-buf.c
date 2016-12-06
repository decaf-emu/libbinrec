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
    alloc_dummy_registers(unit, 1, RTLTYPE_FLOAT32);

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_FLOAT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, UINT64_C(0x3FF0000000000000)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT32));
    EXPECT(rtl_add_insn(unit, RTLOP_FCAST, reg2, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xEB,0x2A,                          // jmp 0x30
    0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (padding)
    0x00,0x00,0x00,                     // (padding)

    0xFF,0xFF,0xBF,0xFF,0x00,0x00,0x00,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)
    0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00, // (data)

    0x48,0xB8,0x00,0x00,0x00,0x00,0x00, // mov $0x3FF0000000000000,%rax
      0x00,0xF0,0x3F,
    0x66,0x48,0x0F,0x6E,0xC8,           // movq %rax,%xmm1
    0x0F,0xAE,0x1C,0x24,                // stmxcsr (%rsp)
    0x66,0x0F,0x2E,0xC9,                // ucomisd %xmm1,%xmm1
    0xF2,0x0F,0x5A,0xD1,                // cvtsd2ss %xmm1,%xmm2
    0x7B,0x15,                          // jnp L0
    0x66,0x48,0x0F,0x7E,0xC8,           // movq %xmm1,%rax
    0x48,0x85,0x05,0xC7,0xFF,0xFF,0xFF, // test %rax,-57(%rip)
    0x75,0x07,                          // jnz L0
    0x0F,0x54,0x15,0xAE,0xFF,0xFF,0xFF, // andps -82(%rip),%xmm2
    0x0F,0xAE,0x14,0x24,                // L0: ldmxcsr (%rsp)
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
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
