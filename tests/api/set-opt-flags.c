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
    EXPECT_EQ(handle->common_opt, 0);
    EXPECT_EQ(handle->guest_opt, 0);
    EXPECT_EQ(handle->host_opt, 0);

    /* Check that we can set flags. */
    binrec_set_optimization_flags(handle,
                                  BINREC_OPT_BASIC | BINREC_OPT_DECONDITION,
                                  BINREC_OPT_G_PPC_NATIVE_RECIPROCAL,
                                  BINREC_OPT_H_X86_STORE_IMMEDIATE);
    EXPECT_EQ(handle->common_opt, BINREC_OPT_BASIC | BINREC_OPT_DECONDITION);
    EXPECT_EQ(handle->guest_opt, BINREC_OPT_G_PPC_NATIVE_RECIPROCAL);
    EXPECT_EQ(handle->host_opt, BINREC_OPT_H_X86_STORE_IMMEDIATE);

    /* Check that flags overwrite the existing settings rather than adding
     * to them. */
    binrec_set_optimization_flags(handle, BINREC_OPT_FOLD_CONSTANTS, 0, 0);
    EXPECT_EQ(handle->common_opt, BINREC_OPT_FOLD_CONSTANTS);
    EXPECT_EQ(handle->guest_opt, 0);
    EXPECT_EQ(handle->host_opt, 0);

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
