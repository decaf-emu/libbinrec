/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef SRC_MEMORY_H
#define SRC_MEMORY_H

#ifndef SRC_COMMON_H
    #include "src/common.h"
#endif

/*************************************************************************/
/*************************************************************************/

/**
 * binrec_code_malloc:  Allocate an output code buffer.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     size: Size of buffer, in bytes.  Must be nonzero.
 *     alignment: Desired buffer alignment, in bytes.  Must be a power of 2.
 * [Return value]
 *     Allocated buffer, or NULL on error.
 */
#define binrec_code_malloc INTERNAL(binrec_code_malloc)
extern void *binrec_code_malloc(const binrec_t *handle, size_t size,
                                size_t alignment);

/**
 * binrec_code_realloc:  Expand an output code buffer.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     ptr: Output code buffer to expand.
 *     old_size: Old size of buffer, in bytes.
 *     new_size: New size of buffer, in bytes.  Must be nonzero.
 *     alignment: Buffer alignment, in bytes.  Must be equal to the value
 *         passed to binrec_code_malloc() when the block was allocated.
 * [Return value]
 *     Expanded buffer, or NULL on error.
 */
#define binrec_code_realloc INTERNAL(binrec_code_realloc)
extern void *binrec_code_realloc(const binrec_t *handle, void *ptr,
                                 size_t old_size, size_t new_size,
                                 size_t alignment);

/**
 * binrec_code_free:  Free an output code buffer.
 *
 * [Parameters]
 *     handle: Translation handle.
 *     ptr: Output code buffer to free.
 */
#define binrec_code_free INTERNAL(binrec_code_free)
extern void binrec_code_free(const binrec_t *handle, void *ptr);

/*************************************************************************/
/*************************************************************************/

#endif  // SRC_MEMORY_H
