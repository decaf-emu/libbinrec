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
#include "tests/common.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    /* Optimizations should default to off. */
    EXPECT_EQ(handle->optimizations, 0);

    /* Check that we can set flags. */
    binrec_set_optimization_flags(
        handle, BINREC_OPT_ENABLE | BINREC_OPT_DECONDITION);
    EXPECT_EQ(handle->optimizations,
              BINREC_OPT_ENABLE | BINREC_OPT_DECONDITION);

    /* Check that flags overwrite the existing settings rather than adding
     * to them. */
    binrec_set_optimization_flags(
        handle, BINREC_OPT_ENABLE | BINREC_OPT_FOLD_CONSTANTS);
    EXPECT_EQ(handle->optimizations,
              BINREC_OPT_ENABLE | BINREC_OPT_FOLD_CONSTANTS);

    /* Check that omitting ENABLE suppresses all other flags. */
    binrec_set_optimization_flags(handle, BINREC_OPT_FOLD_CONSTANTS);
    EXPECT_EQ(handle->optimizations, 0);

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
