/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec++.h"
#include "tests/execute.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* From tests/common.h (which we don't include since it requires
 * src/common.h, which is not C++-safe).  The actual definition uses
 * uint8_t* instead of void*, but we declare it here with void* so we
 * don't need casts below. */
extern "C" void _diff_mem(FILE *f, const void *from, const void *to, long len);

#ifndef VERSION
# error VERSION must be defined to the expected version string.
#endif


static uint8_t memory[0x10000];

/* Dummy structure declarations to verify that binrec++.h properly casts
 * to and from void. */
struct Foo;
struct Bar;

/* Verify that renamed flag values are implemented correctly. */
#define CHECK_FLAG(c_prefix, cxx_namespace, name) \
    static_assert(cxx_namespace::name == c_prefix##_##name, \
                  #cxx_namespace "::" #name " has the wrong value")
CHECK_FLAG(BINREC_FEATURE_X86, binrec::Feature::X86, FMA);
CHECK_FLAG(BINREC_FEATURE_X86, binrec::Feature::X86, MOVBE);
CHECK_FLAG(BINREC_FEATURE_X86, binrec::Feature::X86, LZCNT);
CHECK_FLAG(BINREC_FEATURE_X86, binrec::Feature::X86, BMI1);
CHECK_FLAG(BINREC_FEATURE_X86, binrec::Feature::X86, BMI2);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, BASIC);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, DECONDITION);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, DEEP_DATA_FLOW);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, DSE);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, DSE_FP);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, FOLD_CONSTANTS);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, FOLD_FP_CONSTANTS);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, FOLD_VECTORS);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, NATIVE_IEEE_NAN);
CHECK_FLAG(BINREC_OPT, binrec::Optimize, NATIVE_IEEE_UNDERFLOW);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, ASSUME_NO_SNAN);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, CONSTANT_GQRS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, DETECT_FCFI_EMUL);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, FAST_FCTIW);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, FAST_FMADDS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, FAST_FMULS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, FAST_STFS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, FNMADD_ZERO_SIGN);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, FORWARD_LOADS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, IGNORE_FPSCR_VXFOO);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, NATIVE_RECIPROCAL);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, NO_FPSCR_STATE);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, PAIRED_LWARX_STWCX);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, PS_STORE_DENORMALS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, SINGLE_PREC_INPUTS);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, TRIM_CR_STORES);
CHECK_FLAG(BINREC_OPT_G_PPC, binrec::Optimize::GuestPPC, USE_SPLIT_FIELDS);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, ADDRESS_OPERANDS);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, BRANCH_ALIGNMENT);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, CONDITION_CODES);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, FIXED_REGS);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, FORWARD_CONDITIONS);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, MERGE_REGS);
CHECK_FLAG(BINREC_OPT_H_X86, binrec::Optimize::HostX86, STORE_IMMEDIATE);


extern "C" int main(void)
{
    const char *version = binrec::version();
    if (strcmp(version, VERSION) != 0) {
        printf("%s:%d: binrec::version() was \"%s\" but should have been \"%s\"\n",
               __FILE__, __LINE__, version, VERSION);
        return EXIT_FAILURE;
    }

    const binrec::Arch native_arch = binrec::native_arch();
    binrec::Arch expected_native_arch;
    #if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
        #ifdef _WIN32
            expected_native_arch = BINREC_ARCH_X86_64_WINDOWS;
        #else
            expected_native_arch = BINREC_ARCH_X86_64_SYSV;
        #endif
    #else
        expected_native_arch = BINREC_ARCH_INVALID;
    #endif
    if (native_arch != expected_native_arch) {
        printf("%s:%d: binrec::native_arch() was %d but should have been %d\n",
               __FILE__, __LINE__, (int)native_arch, (int)expected_native_arch);
        return EXIT_FAILURE;
    }

    const unsigned int native_features = binrec::native_features();
    #if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64)
        /* As with the C binrec_native_features() test, we assume that
         * at least one feature flag will be present for x86 systems. */
        if (native_features == 0) {
            printf("%s:%d: binrec::native_features() was 0 but should have been nonzero\n",
                   __FILE__, __LINE__);
            return EXIT_FAILURE;
        }
    #else
        if (native_features != 0) {
            printf("%s:%d: binrec::native_features() was %u but should have been 0\n",
                   __FILE__, __LINE__, native_features);
            return EXIT_FAILURE;
        }
    #endif

    if (!binrec::guest_supported(BINREC_ARCH_PPC_7XX)) {
        printf("%s:%d: binrec::guest_supported(BINREC_ARCH_PPC_7XX) was not true as expected\n",
               __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    if (!binrec::host_supported(BINREC_ARCH_X86_64_SYSV)) {
        printf("%s:%d: binrec::host_supported(BINREC_ARCH_X86_64_SYSV) was not true as expected\n",
               __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    binrec::Setup setup;
    memset(&setup, 0, sizeof(setup));
    setup.guest = BINREC_ARCH_PPC_7XX;
    setup.host = BINREC_ARCH_X86_64_SYSV;
    setup.guest_memory_base = memory;
    ppc32_fill_setup(&setup);

    binrec::Handle<Foo *, Bar *> handle;
    if (!handle.initialize(setup)) {
        printf("%s:%d: handle.initialize(setup) was not true as expected\n",
               __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

    /* Call all the functions just to check that they're present. */
    handle.set_code_range(0, -1);
    handle.set_optimization_flags(0, 0, 0);
    handle.set_max_inline_length(0);
    handle.set_max_inline_depth(0);
    handle.add_readonly_region(0, 1);
    handle.clear_readonly_regions();
    handle.enable_chaining(false);
    handle.enable_branch_exit_test(false);
    handle.set_pre_insn_callback(nullptr);
    handle.set_post_insn_callback(nullptr);
    handle.enable_verify(false);

    static const uint8_t ppc_code[] = {
        0x38,0x60,0x00,0x01,  // li r3,1
        0x38,0x80,0x00,0x0A,  // li r4,10
    };
    const uint32_t start_address = 0x1000;
    const uint32_t end_address = start_address + sizeof(ppc_code) - 1;
    memcpy(memory + start_address, ppc_code, sizeof(ppc_code));

    Bar *x86_code;
    long x86_code_size;
    if (!handle.translate(nullptr, start_address, end_address,
                          &x86_code, &x86_code_size)) {
        printf("%s:%d: handle.translate(...) was not true as expected\n",
               __FILE__, __LINE__);
        return EXIT_FAILURE;
    }

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
    if (memcmp(x86_code, x86_expected, sizeof(x86_expected)) != 0) {
        printf("%s:%d: x86_code differed from the expected data:\n",
               __FILE__, __LINE__);
        _diff_mem(stdout, x86_expected, x86_code, sizeof(x86_expected));
        return EXIT_FAILURE;
    }
    if (x86_code_size != (long)sizeof(x86_expected)) {
        printf("%s:%d: x86_code_size was %ld but should have been %zd\n",
               __FILE__, __LINE__, x86_code_size, sizeof(x86_expected));
        return EXIT_FAILURE;
    }

    free(x86_code);
    return EXIT_SUCCESS;
}
