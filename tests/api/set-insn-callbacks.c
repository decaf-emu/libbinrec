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
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    EXPECT_FALSE(handle->pre_insn_callback);
    EXPECT_FALSE(handle->post_insn_callback);

    binrec_set_pre_insn_callback(handle, (void (*)(void *, uint32_t))1);
    EXPECT_PTREQ(handle->pre_insn_callback, (void (*)(void *, uint32_t))1);
    EXPECT_FALSE(handle->post_insn_callback);

    binrec_set_post_insn_callback(handle, (void (*)(void *, uint32_t))2);
    EXPECT_PTREQ(handle->pre_insn_callback, (void (*)(void *, uint32_t))1);
    EXPECT_PTREQ(handle->post_insn_callback, (void (*)(void *, uint32_t))2);

    binrec_set_pre_insn_callback(handle, NULL);
    EXPECT_FALSE(handle->pre_insn_callback);
    EXPECT_PTREQ(handle->post_insn_callback, (void (*)(void *, uint32_t))2);

    binrec_set_post_insn_callback(handle, NULL);
    EXPECT_FALSE(handle->pre_insn_callback);
    EXPECT_FALSE(handle->post_insn_callback);

    EXPECT_STREQ(get_log_messages(), NULL);

    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
