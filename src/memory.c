/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/common.h"

/* Disable malloc() suppression from common.h. */
#undef malloc
#undef realloc
#undef free

/*************************************************************************/
/*************** Output buffer memory management routines ****************/
/*************************************************************************/

void *binrec_code_malloc(const binrec_t *handle, size_t size, size_t alignment)
{
    ASSERT(handle);
    ASSERT(size > 0);
    ASSERT(alignment > 0);
    ASSERT((alignment & (alignment - 1)) == 0);  // Ensure it's a power of 2.

    if (handle->setup.code_malloc) {
        return (*handle->setup.code_malloc)(
            handle->setup.userdata, size, alignment);
    }

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    /* We store the actual pointer returned by malloc() immediately before
     * the aligned block, so we can pass it to realloc() or free() later. */
    size += sizeof(void *);

    if (alignment <= sizeof(void *)) {
        /* malloc() guarantees at least pointer-size alignment, so we don't
         * have to do anything special. */
        void **base = binrec_malloc(handle, size);
        if (UNLIKELY(!base)) {
            return NULL;
        }
        *base = base;
        return &base[1];
    } else {
        /* We need to align the buffer manually.  We use "char *" here for
         * convenience, since the C standard guarantees that sizeof(char)
         * is 1. */
        char *base = binrec_malloc(handle, size + (alignment - 1));
        if (UNLIKELY(!base)) {
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

void *binrec_code_realloc(const binrec_t *handle, void *ptr, size_t old_size,
                          size_t new_size, size_t alignment)
{
    ASSERT(handle);
    ASSERT(ptr);
    ASSERT(old_size > 0);
    ASSERT(new_size > 0);
    ASSERT(alignment > 0);
    ASSERT((alignment & (alignment - 1)) == 0);

    if (handle->setup.code_realloc) {
        return (*handle->setup.code_realloc)(
            handle->setup.userdata, ptr, old_size, new_size, alignment);
    }

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    void *base = ((void **)ptr)[-1];
    new_size += sizeof(void *);
    if (alignment <= sizeof(void *)) {
        void **new_base = binrec_realloc(handle, base, new_size);
        if (UNLIKELY(!new_base)) {
            return NULL;
        }
        *new_base = new_base;
        return &new_base[1];
    } else {
        char *new_base =
            binrec_realloc(handle, base, new_size + (alignment - 1));
        if (UNLIKELY(!new_base)) {
            return NULL;
        }
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

void binrec_code_free(const binrec_t *handle, void *ptr)
{
    ASSERT(handle);

    if (handle->setup.code_free) {
        return (*handle->setup.code_free)(
            handle->setup.userdata, ptr);
    }

    binrec_free(handle, ((void **)ptr)[-1]);
}

/*************************************************************************/
/*************************************************************************/
