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

bool binrec_expand_code_buffer(binrec_t *handle, long new_size)
{
    ASSERT(handle);
    ASSERT(handle->code_buffer);
    ASSERT(new_size > handle->code_buffer_size);

    new_size = ((new_size + CODE_EXPAND_SIZE - 1)
                / CODE_EXPAND_SIZE) * CODE_EXPAND_SIZE;
    void *new_buffer = binrec_code_realloc(
        handle, handle->code_buffer, handle->code_buffer_size, new_size,
        handle->code_alignment);
    if (!new_buffer) {
        return false;
    }
    handle->code_buffer = new_buffer;
    handle->code_buffer_size = new_size;
    return true;
}

/*************************************************************************/
/*************************************************************************/
