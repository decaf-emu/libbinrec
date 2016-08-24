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
 * Entry:  Function type of generated native code.  Wraps binrec_entry_t.
 */
using Entry = ::binrec_entry_t;

/**
 * LogLevel:  Enumeration of log message levels.  Wraps binrec_loglevel_t.
 */
using LogLevel = ::binrec_loglevel_t;

/**
 * Optimize:  Namespace wrapping optimization flags.
 */
namespace Optimize {
    const unsigned int ENABLE = BINREC_OPT_ENABLE;
    const unsigned int CALLEE_SAVED_REGS_UNSAFE = BINREC_OPT_CALLEE_SAVED_REGS_UNSAFE;
    const unsigned int DROP_DEAD_BLOCKS = BINREC_OPT_DROP_DEAD_BLOCKS;
    const unsigned int DROP_DEAD_BRANCHES = BINREC_OPT_DROP_DEAD_BRANCHES;
    const unsigned int DECONDITION = BINREC_OPT_DECONDITION;
    const unsigned int FOLD_CONSTANTS = BINREC_OPT_FOLD_CONSTANTS;
    const unsigned int NATIVE_CALLS = BINREC_OPT_NATIVE_CALLS;
    const unsigned int STACK_FRAMES_UNSAFE = BINREC_OPT_STACK_FRAMES_UNSAFE;
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
    void set_optimization_flags(unsigned int flags) {
        ::binrec_set_optimization_flags(handle, flags);
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
    bool translate(uint32_t address, Entry *native_code_ret,
                   size_t *native_size_ret) {
        return bool(::binrec_translate(handle, address, native_code_ret,
                                       native_size_ret));
    }

  private:
    binrec_t *handle;
};

/**
 * version:  Return the version number of the library as a string.
 * Wraps binrec_version().
 */
static inline const char *version() {return ::binrec_version();}

/*************************************************************************/
/*************************************************************************/

}  // namespace binrec

#endif  // BINRECXX_H
