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

#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/*********************** Default memory allocator ************************/
/*************************************************************************/

static void *default_native_malloc(
    UNUSED void *userdata, size_t size, size_t alignment)
{
    ASSERT(alignment > 0);
    ASSERT((alignment & (alignment - 1)) == 0);  // Ensure it's a power of 2.

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    /* We store the actual pointer returned by malloc() immediately before
     * the aligned block, so we can pass it to realloc() or free() later. */
    size += sizeof(void *);

    if (alignment <= sizeof(void *)) {
        /* malloc() guarantees at least pointer-size alignment, so we don't
         * have to do anything special. */
        void **base = malloc(size);
        if (!base) {
            return NULL;
        }
        *base = base;
        return &base[1];
    } else {
        /* We need to align the buffer manually.  We use "char *" here for
         * convenience, since the C standard guarantees that sizeof(char)
         * is 1. */
        char *base = malloc(size + (alignment - 1));
        if (!base) {
            return NULL;
        }
        /* This alignment operation will always leave at least one pointer's
         * worth of space before the aligned address.  If malloc() returned
         * an address with the correct alignment, we add one alignment unit
         * (which here is greater than the size of a pointer) to the base
         * address; otherwise, since malloc() returns an address aligned at
         * least to the size of a pointer, there must be at least one
         * pointer's difference between the base address and the next
         * aligned address. */
        char *ptr = base + (alignment - ((uintptr_t)base % alignment));
        ((void **)ptr)[-1] = base;
        return ptr;
    }
}

/*-----------------------------------------------------------------------*/

static void *default_native_realloc(
    UNUSED void *userdata, void *ptr, size_t old_size, size_t new_size,
    size_t alignment)
{
    ASSERT(ptr);
    ASSERT(alignment > 0);
    ASSERT((alignment & (alignment - 1)) == 0);  // Ensure it's a power of 2.

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    void *base = ((void **)ptr)[-1];
    new_size += sizeof(void *);
    if (alignment <= sizeof(void *)) {
        void **new_base = realloc(base, new_size);
        if (!new_base) {
            return NULL;
        }
        *new_base = new_base;
        return &new_base[1];
    } else {
        char *new_base = realloc(base, new_size + (alignment - 1));
        char *new_ptr =
            new_base + (alignment - ((uintptr_t)new_base % alignment));
        const size_t new_offset = new_ptr - new_base;
        const size_t old_offset = (uintptr_t)ptr - (uintptr_t)base;
        if (new_offset != old_offset) {
            /* The alignment changed, so we have to move the data. */
            const size_t move_size = min(old_size, new_size);
            memmove(new_ptr, new_base + old_offset, move_size);
        }
        ((void **)new_ptr)[-1] = new_base;
        return new_ptr;
    }
}

/*-----------------------------------------------------------------------*/

static void default_native_free(UNUSED void *userdata, void *ptr)
{
    free(((void **)ptr)[-1]);
}

/*************************************************************************/
/************************ Other helper functions *************************/
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
    binrec_t *handle = calloc(1, sizeof(*handle));
    if (!handle) {
        return NULL;
    }

    handle->setup = *setup;
    const int have_malloc = (setup->native_malloc != NULL);
    const int have_realloc = (setup->native_realloc != NULL);
    const int have_free = (setup->native_free != NULL);
    if (have_malloc + have_realloc + have_free != 3) {
        if (have_malloc + have_realloc + have_free > 0) {
            log_warning(handle, "Some but not all memory allocation functions"
                        " were defined.  Using default allocator instead.");
        }
        handle->setup.native_malloc = default_native_malloc;
        handle->setup.native_realloc = default_native_realloc;
        handle->setup.native_free = default_native_free;
    }

    handle->max_inline_depth = 1;
    memset(handle->partial_readonly_pages, 0xFF,
           sizeof(handle->partial_readonly_pages));

    return handle;
}

/*-----------------------------------------------------------------------*/

void binrec_destroy_handle(binrec_t *handle)
{
    free(handle);
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
        log_error(handle, "Invalid max inline length %d", length);
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
    binrec_t *handle, uint32_t address, uint32_t *src_length_ret,
    binrec_entry_t *native_code_ret, size_t *native_size_ret)
{
    ASSERT(handle);

    return 0;  // FIXME: not yet implemented
}

/*************************************************************************/
/*************************************************************************/
