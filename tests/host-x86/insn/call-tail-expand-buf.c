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
    alloc_dummy_registers(unit, 1, RTLTYPE_INT32);

    int reg1;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 1));
    rtl_make_unfoldable(unit, reg1);
    EXPECT(rtl_add_insn(unit, RTLOP_CALL, 0, reg1, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));

    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp
    0xB9,0x01,0x00,0x00,0x00,           // mov $1,%ecx
    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xFF,0xE1,                          // jmp *%rcx
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
