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

    /* Always align to at least the size of a pointer, since we store the
     * base address returned from binrec_malloc() immediately before the
     * address we return to our caller (for passing to binrec_realloc() or
     * binrec_free() later). */
    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }

    /* Allocate the buffer, leaving enough space for alignment and for
     * saving the base pointer.  We use "char *" here for convenience,
     * since the C standard guarantees that sizeof(char) is 1. */
    char *base = binrec_malloc(
        handle, size + sizeof(void *) + (alignment - 1));
    if (UNLIKELY(!base)) {
        return NULL;
    }

    /* Align the pointer to be returned. */
    char *ptr = base + sizeof(void *);
    if ((uintptr_t)ptr % alignment != 0) {
        ptr += alignment - ((uintptr_t)ptr % alignment);
    }

    /* Save the base pointer and return. */
    ((void **)ptr)[-1] = base;
    return ptr;
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
    char *new_base = binrec_realloc(
        handle, base, new_size + sizeof(void *) + (alignment - 1));
    if (UNLIKELY(!new_base)) {
        return NULL;
    }

    char *new_ptr = new_base + sizeof(void *);
    if ((uintptr_t)new_ptr % alignment != 0) {
        new_ptr += alignment - ((uintptr_t)new_ptr % alignment);
    }

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
