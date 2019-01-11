/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "tests/common.h"
#include "tests/execute.h"
#include "tests/log-capture.h"


static uint8_t memory[0x10000];


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = BINREC_ARCH_PPC_7XX;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.guest_memory_base = memory;
    ppc32_fill_setup(&setup);
    setup.log = log_capture;

    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    static const uint8_t ppc_code[] = {
        0x38,0x60,0x00,0x01,  // li r3,1
        0x38,0x80,0x00,0x0A,  // li r4,10
    };
    const uint32_t start_address = 0x1000;
    const uint32_t end_address = start_address + sizeof(ppc_code) - 1;
    memcpy(memory + start_address, ppc_code, sizeof(ppc_code));

    void *x86_code;
    long x86_code_size;
    EXPECT(binrec_translate(handle, NULL, start_address, end_address,
                            &x86_code, &x86_code_size));

    static const uint8_t x86_expected[] = {
        0x48,0x83,0xEC,0x08,            // sub $8,%rsp
        0xB8,0x01,0x00,0x00,0x00,       // mov $1,%eax
        0x89,0x47,0x0C,                 // mov %eax,12(%rdi)
        0xB8,0x0A,0x00,0x00,0x00,       // mov $10,%ecx
        0x89,0x47,0x10,                 // mov %ecx,16(%rdi)
        0xB8,0x08,0x10,0x00,0x00,       // mov $0x1008,%eax
        0x89,0x87,0xC4,0x02,0x00,0x00,  // mov %eax,708(%rdi)
        0x48,0x8B,0xC7,                 // mov %rdi,%rax
        0x48,0x83,0xC4,0x08,            // add $8,%rsp
        0xC3,                           // ret
    };
    EXPECT_MEMEQ(x86_code, x86_expected, sizeof(x86_expected));
    EXPECT_EQ(x86_code_size, sizeof(x86_expected));

    EXPECT_STREQ(get_log_messages(), "[info] Scanning terminated at requested"
                 " limit 0x1007\n");

    free(x86_code);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
