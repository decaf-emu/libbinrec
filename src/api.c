/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/common.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"

/* Disable malloc() suppression from common.h. */
#undef malloc
#undef realloc
#undef free

#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/*************************** Helper functions ****************************/
/*************************************************************************/

/**
 * add_partial_readonly_page:  Mark the given partial page as read-only.
 * Helper for binrec_add_readonly_region().
 */
static int add_partial_readonly_page(binrec_t *handle,
                                     uint32_t start, uint32_t end)
{
    ASSERT(handle);
    ASSERT(end >= start);

    const uint32_t page = start & ~READONLY_PAGE_MASK;

    int index;
    for (index = 0; index < MAX_PARTIAL_READONLY; index++) {
        if (page <= handle->partial_readonly_pages[index]) {
            break;
        }
    }
    if (index == MAX_PARTIAL_READONLY) {
        log_error(handle, "Failed to add read-only range [0x%X,0x%X]: no free"
                  " partial page entries", start, end-1);
        return 0;
    }

    if (page != handle->partial_readonly_pages[index]) {
        if (handle->partial_readonly_pages[MAX_PARTIAL_READONLY - 1] != ~0U) {
            log_error(handle, "Failed to add read-only range [0x%X,0x%X]: no"
                      " free partial page entries", start, end-1);
            return 0;
        }
        memmove(&handle->partial_readonly_pages[index],
                &handle->partial_readonly_pages[index+1],
                sizeof(*handle->partial_readonly_pages) * ((MAX_PARTIAL_READONLY-1) - index));
        memmove(&handle->partial_readonly_map[index],
                &handle->partial_readonly_map[index+1],
                sizeof(*handle->partial_readonly_map) * ((MAX_PARTIAL_READONLY-1) - index));
        handle->partial_readonly_pages[index] = page;
        memset(handle->partial_readonly_map[index], 0,
               sizeof(*handle->partial_readonly_map));
        const uint32_t page_index = page >> READONLY_PAGE_BITS;
        handle->readonly_map[page_index>>2] |= 1 << ((page_index & 3) * 2);
    }

    start -= page;
    end -= page;
    ASSERT(end <= READONLY_PAGE_SIZE);
    for (uint32_t address = start; address < end; address++) {
        handle->partial_readonly_map[index][address>>3] |= 1 << (address & 7);
    }
    return 1;
}

/*************************************************************************/
/************************** Interface functions **************************/
/*************************************************************************/

const char *binrec_version(void)
{
    return VERSION;
}

/*-----------------------------------------------------------------------*/

binrec_t *binrec_create_handle(const binrec_setup_t *setup)
{
    ASSERT(setup);

    binrec_t *handle;
    if (setup->malloc) {
        handle = (*setup->malloc)(setup->userdata, sizeof(*handle));
    } else {
        handle = malloc(sizeof(*handle));
    }
    if (UNLIKELY(!handle)) {
        return NULL;
    }

    memset(handle, 0, sizeof(*handle));
    handle->setup = *setup;

    const int have_malloc = (setup->malloc != NULL);
    const int have_realloc = (setup->realloc != NULL);
    const int have_free = (setup->free != NULL);
    if (have_malloc + have_realloc + have_free != 3
     && have_malloc + have_realloc + have_free > 0) {
        log_warning(handle, "Some but not all memory allocation functions"
                    " were defined.  Using system allocator instead.");
        handle->setup.malloc = NULL;
        handle->setup.realloc = NULL;
        handle->setup.free = NULL;
    }

    const int have_code_malloc = (setup->code_malloc != NULL);
    const int have_code_realloc = (setup->code_realloc != NULL);
    const int have_code_free = (setup->code_free != NULL);
    if (have_code_malloc + have_code_realloc + have_code_free != 3
     && have_code_malloc + have_code_realloc + have_code_free > 0) {
        log_warning(handle, "Some but not all output code memory allocation"
                    " functions were defined.  Using default allocator"
                    " instead.");
        handle->setup.code_malloc = NULL;
        handle->setup.code_realloc = NULL;
        handle->setup.code_free = NULL;
    }

    handle->code_range_end = -1;
    handle->max_inline_depth = 1;
    memset(handle->partial_readonly_pages, 0xFF,
           sizeof(handle->partial_readonly_pages));

    return handle;
}

/*-----------------------------------------------------------------------*/

void binrec_destroy_handle(binrec_t *handle)
{
    binrec_free(handle, handle);
}

/*-----------------------------------------------------------------------*/

void binrec_set_code_range(binrec_t *handle, uint32_t start, uint32_t end)
{
    ASSERT(handle);

    if (UNLIKELY(end < start)) {
        log_error(handle, "Invalid code range 0x%X-0x%X");
        /* Signal an invalid state to binrec_translate(). */
        handle->code_range_start = 1;
        handle->code_range_end = 0;
        return;
    }

    handle->code_range_start = start;
    handle->code_range_end = end;
}

/*-----------------------------------------------------------------------*/

void binrec_set_optimization_flags(binrec_t *handle, unsigned int flags)
{
    ASSERT(handle);

    if (flags & BINREC_OPT_ENABLE) {
        handle->optimizations = flags;
    } else {
        handle->optimizations = 0;
    }
}

/*-----------------------------------------------------------------------*/

void binrec_set_max_inline_length(binrec_t *handle, int length)
{
    ASSERT(handle);

    if (length >= 0) {
        handle->max_inline_length = length;
    } else {
        log_error(handle, "Invalid maximum inline length %d", length);
    }
}

/*-----------------------------------------------------------------------*/

void binrec_set_max_inline_depth(binrec_t *handle, int depth)
{
    ASSERT(handle);

    if (depth >= 1) {
        handle->max_inline_depth = depth;
    } else {
        log_error(handle, "Invalid max inline depth %d", depth);
    }
}

/*-----------------------------------------------------------------------*/

int binrec_add_readonly_region(binrec_t *handle, uint32_t base, uint32_t size)
{
    ASSERT(handle);

    uint32_t top = base + size;

    if ((base & READONLY_PAGE_MASK) != 0) {
        const uint32_t page_top =
            min((base & ~READONLY_PAGE_MASK) + READONLY_PAGE_SIZE, top);
        if (!add_partial_readonly_page(handle, base, page_top)) {
            return 0;
        }
        base = page_top;
        if (base == top) {
            return 1;
        }
    }

    if ((top & READONLY_PAGE_MASK) != 0) {
        const uint32_t page_base = top & ~READONLY_PAGE_MASK;
        if (!add_partial_readonly_page(handle, page_base, top)) {
            return 0;
        }
    }

    base >>= READONLY_PAGE_BITS;
    top >>= READONLY_PAGE_BITS;
    for (uint32_t page = base; page < top; page++) {
        handle->readonly_map[page>>2] &= ~(1 << ((page & 3) * 2));
        handle->readonly_map[page>>2] |= 2 << ((page & 3) * 2);
    }
    return 1;
}

/*-----------------------------------------------------------------------*/

void binrec_clear_readonly_regions(binrec_t *handle)
{
    ASSERT(handle);

    memset(handle->readonly_map, 0, sizeof(handle->readonly_map));
    memset(handle->partial_readonly_pages, 0xFF,
           sizeof(handle->partial_readonly_pages));
}

/*-----------------------------------------------------------------------*/

int binrec_translate(
    binrec_t *handle, uint32_t address, binrec_entry_t *native_code_ret,
    long *native_size_ret)
{
    ASSERT(handle);

    if (UNLIKELY(handle->code_range_end < handle->code_range_start)) {
        log_error(handle, "Code range invalid");
        return 0;
    }
    if (UNLIKELY(address < handle->code_range_start)
     || UNLIKELY(address > handle->code_range_end)) {
        log_error(handle, "Address 0x%X not within code range 0x%X-0x%X",
                  address, handle->code_range_start, handle->code_range_end);
        return 0;
    }

    RTLUnit *unit = rtl_create_unit(handle);
    if (UNLIKELY(!unit)) {
        log_error(handle, "Failed to create RTLUnit");
        return 0;
    }

    if (!guest_ppc_translate(handle, address, unit)) {
        log_error(handle, "Failed to parse guest instructions starting at"
                  " 0x%X", address);
        return 0;
    }

    if (!rtl_finalize_unit(unit)) {
        log_error(handle, "Failed to finalize RTL for code at 0x%X", address);
        rtl_destroy_unit(unit);
        return 0;
    }

    if (!rtl_optimize_unit(unit, handle->optimizations)) {
        log_warning(handle, "Failed to optimize RTL for code at 0x%X",
                    address);
        /* Don't treat this as an error; just translate the unoptimized
         * unit. */
    }

#if 0  // FIXME: not yet implemented
    const bool result = host_x86_translate(handle, unit,
                                           native_code_ret, native_size_ret);
    rtl_destroy_unit(unit);
    if (!result) {
        log_error(handle, "Failed to generate host code for 0x%X", address);
        return 0;
    }

    return 1;
#else
    return 0;
#endif
}

/*************************************************************************/
/*************************************************************************/
