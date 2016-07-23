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
# error This header can only be used with C++.
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
    const unsigned int DROP_DEAD_BLOCKS = BINREC_OPT_DROP_DEAD_BLOCKS;
    const unsigned int DROP_DEAD_BRANCHES = BINREC_OPT_DROP_DEAD_BRANCHES;
    const unsigned int DECONDITION = BINREC_OPT_DECONDITION;
    const unsigned int FOLD_CONSTANTS = BINREC_OPT_FOLD_CONSTANTS;
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
     * handle.
     *
     * [Parameters]
     *     setup: Handle parameters, as for binrec_create_handle().
     * [Return value]
     *     True if handle was successfully initialized, false if not.
     */
    bool initialize(const Setup &setup) {
        if (!handle) {
            handle = ::binrec_create_handle(&setup);
            return handle != nullptr;
        }
    }

    /**
     * set_optimization_flags:  Set which optimizations should be performed
     * on translated blocks.  Wraps binrec_set_optimization_flags().
     */
    void set_optimization_flags(unsigned int flags) {
        ::binrec_set_optimization_flags(handle, flags);
    }

    /**
     * add_readonly_region:  Mark the given region of memory as read-only.
     * Wraps binrec_add_readonly_region().
     */
    bool add_readonly_region(uint32_t base, uint32_t size) {
        return 0 != ::binrec_add_readonly_region(handle, base, size);
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
    bool translate(uint32_t address, uint32_t *src_length_ret,
                   Entry *native_code_ret, size_t *native_size_ret) {
        return 0 != ::binrec_translate(handle, address, src_length_ret,
                                       native_code_ret, native_size_ret);
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
