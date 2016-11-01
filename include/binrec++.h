/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef BINRECXX_H
#define BINRECXX_H

#ifndef __cplusplus
    #error This header can only be used with C++.
#endif

#include "binrec.h"

namespace binrec {

/*************************************************************************/
/*************************************************************************/

/**
 * Arch:  Enumeration of supported architectures.  Wraps binrec_arch_t.
 */
using Arch = ::binrec_arch_t;

/**
 * LogLevel:  Enumeration of log message levels.  Wraps binrec_loglevel_t.
 */
using LogLevel = ::binrec_loglevel_t;

/**
 * Feature:  Namespace wrapping architecture feature flags.
 */
namespace Feature {
    namespace X86 {
        const unsigned int FMA = BINREC_FEATURE_X86_FMA;
        const unsigned int MOVBE = BINREC_FEATURE_X86_MOVBE;
        const unsigned int POPCNT = BINREC_FEATURE_X86_POPCNT;
        const unsigned int AVX = BINREC_FEATURE_X86_AVX;
        const unsigned int LZCNT = BINREC_FEATURE_X86_LZCNT;
        const unsigned int BMI1 = BINREC_FEATURE_X86_BMI1;
        const unsigned int AVX2 = BINREC_FEATURE_X86_AVX2;
        const unsigned int BMI2 = BINREC_FEATURE_X86_BMI2;
    }
}

/**
 * Optimize:  Namespace wrapping optimization flags.
 */
namespace Optimize {
    const unsigned int BASIC = BINREC_OPT_BASIC;
    const unsigned int CALLEE_SAVED_REGS = BINREC_OPT_CALLEE_SAVED_REGS;
    const unsigned int DECONDITION = BINREC_OPT_DECONDITION;
    const unsigned int DEEP_DATA_FLOW = BINREC_OPT_DEEP_DATA_FLOW;
    const unsigned int DSE = BINREC_OPT_DSE;
    const unsigned int FOLD_CONSTANTS = BINREC_OPT_FOLD_CONSTANTS;
    const unsigned int FOLD_FP_CONSTANTS = BINREC_OPT_FOLD_FP_CONSTANTS;
    const unsigned int NATIVE_CALLS = BINREC_OPT_NATIVE_CALLS;
    const unsigned int NATIVE_IEEE_NAN = BINREC_OPT_NATIVE_IEEE_NAN;
    const unsigned int NATIVE_IEEE_TINY = BINREC_OPT_NATIVE_IEEE_TINY;
    const unsigned int STACK_FRAMES = BINREC_OPT_STACK_FRAMES;

    namespace GuestPPC {
        const unsigned int CONSTANT_GQRS = BINREC_OPT_G_PPC_CONSTANT_GQRS;
        const unsigned int NATIVE_RECIPROCAL = BINREC_OPT_G_PPC_NATIVE_RECIPROCAL;
        const unsigned int TRIM_CR_STORES = BINREC_OPT_G_PPC_TRIM_CR_STORES;
    }

    namespace HostX86 {
        const unsigned int ADDRESS_OPERANDS = BINREC_OPT_H_X86_ADDRESS_OPERANDS;
        const unsigned int BRANCH_ALIGNMENT = BINREC_OPT_H_X86_BRANCH_ALIGNMENT;
        const unsigned int CONDITION_CODES = BINREC_OPT_H_X86_CONDITION_CODES;
        const unsigned int FIXED_REGS = BINREC_OPT_H_X86_FIXED_REGS;
        const unsigned int FORWARD_CONDITIONS = BINREC_OPT_H_X86_FORWARD_CONDITIONS;
        const unsigned int MERGE_REGS = BINREC_OPT_H_X86_MERGE_REGS;
        const unsigned int STORE_IMMEDIATE = BINREC_OPT_H_X86_STORE_IMMEDIATE;
    }
}

/**
 * Handle:  Class representing a translation handle.  Wraps binrec_t.
 */
class Handle {

  public:

    /**
     * Setup:  Parameter block for initialize().  Wraps binrec_setup_t.
     */
    using Setup = ::binrec_setup_t;

    Handle(): handle(nullptr) {}
    ~Handle() {::binrec_destroy_handle(handle);}

    /**
     * initialize:  Initialize the handle.  Wraps binrec_create_handle().
     * This method must be called before calling any other methods on the
     * handle, and must not be called again once it has succeeded.
     *
     * [Parameters]
     *     setup: Handle parameters, as for binrec_create_handle().
     * [Return value]
     *     True if handle was successfully initialized, false if not.
     */
    bool initialize(const Setup &setup) {
        ASSERT(!handle);
        handle = ::binrec_create_handle(&setup);
        return handle != nullptr;
    }

    /**
     * set_code_range:  Set the range of addresses from which to read
     * source machine instructions.  Wraps binrec_set_code_range().
     */
    void set_code_range(uint32_t start, uint32_t end) {
        ::binrec_set_code_range(handle, start, end);
    }

    /**
     * set_optimization_flags:  Set which optimizations should be performed
     * on translated blocks.  Wraps binrec_set_optimization_flags().
     */
    void set_optimization_flags(unsigned int common_opt,
                                unsigned int guest_opt,
                                unsigned int host_opt) {
        ::binrec_set_optimization_flags(handle,
                                        common_opt, guest_opt, host_opt);
    }

    /**
     * set_max_inline_length:  Set the maximum length of subroutines to
     * inline.  Wraps binrec_set_max_inline_length().
     */
    void set_max_inline_length(int length) {
        ::binrec_set_max_inline_length(handle, length);
    }

    /**
     * set_max_inline_depth:  Set the maximum depth of subroutines to
     * inline.  Wraps binrec_set_max_inline_depth().
     */
    void set_max_inline_depth(int depth) {
        ::binrec_set_max_inline_depth(handle, depth);
    }

    /**
     * add_readonly_region:  Mark the given region of memory as read-only.
     * Wraps binrec_add_readonly_region().
     */
    bool add_readonly_region(uint32_t base, uint32_t size) {
        return bool(::binrec_add_readonly_region(handle, base, size));
    }

    /**
     * clear_readonly_regions:  Clear all read-only memory regions.
     * Wraps binrec_clear_readonly_regions().
     */
    void clear_readonly_regions() {
        ::binrec_clear_readonly_regions(handle);
    }

    /**
     * translate:  Translate a block of guest machine code into native
     * machine code.  Wraps binrec_translate().
     */
    bool translate(uint32_t address, uint32_t limit,
                   void **code_ret, long *size_ret) {
        return bool(::binrec_translate(handle, address, limit,
                                       code_ret, size_ret));
    }

  private:
    binrec_t *handle;
};

/**
 * version:  Return the version number of the library as a string.
 * Wraps binrec_version().
 */
static inline const char *version() {return ::binrec_version();}

/**
 * native_arch:  Return the architecture of the runtime environment.
 * Wraps binrec_native_arch().
 */
extern Arch native_arch() {return ::binrec_native_arch();}

/**
 * native_features:  Return a bitmask of architecture features available
 * in the runtime environment.  Wraps binrec_native_features().
 */
extern unsigned int native_features() {return ::binrec_native_features();}

/**
 * guest_supported:  Return whether the given architecture is supported as
 * a guest architecure.  Wraps binrec_guest_supported().
 */
extern bool guest_supported(Arch arch) {return ::binrec_guest_supported(arch);}

/**
 * host_supported:  Return whether the given architecture is supported as
 * a host architecure.  Wraps binrec_host_supported().
 */
extern bool host_supported(Arch arch) {return ::binrec_host_supported(arch);}

/*************************************************************************/
/*************************************************************************/

}  // namespace binrec

#endif  // BINRECXX_H
