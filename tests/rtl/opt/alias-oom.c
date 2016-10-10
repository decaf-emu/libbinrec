/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl-internal.h"
#include "tests/common.h"


static unsigned int opt_flags = BINREC_OPT_DEEP_DATA_FLOW;

static int add_rtl(RTLUnit *unit)
{
    int label, alias, reg1, reg2, reg3;
    EXPECT(label = rtl_alloc_label(unit));
    EXPECT(alias = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg2, 0, 0, 1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg1, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg3, 0, 0, alias));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, reg3, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "[warning] No memory for alias tracking, skipping data flow analysis\n"
    "    0: LOAD_ARG   r1, 0\n"
    "    1: LOAD_ARG   r2, 1\n"
    "    2: SET_ALIAS  a1, r1\n"
    "    3: SET_ALIAS  a1, r2\n"
    "    4: LABEL      L1\n"
    "    5: GET_ALIAS  r3, a1\n"
    "    6: RETURN     r3\n"
    "\n"
    "Alias 1: int32, no bound storage\n"
    "\n"
    "Block 0: <none> --> [0,3] --> 1\n"
    "Block 1: 0 --> [4,6] --> <none>\n"
    ;


/* Tweaked version of tests/rtl-optimize-test.i to trigger OOM. */

#include "src/common.h"
#include "src/rtl.h"
#include "tests/common.h"
#include "tests/log-capture.h"
#include "tests/mem-wrappers.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.malloc = mem_wrap_malloc,
    setup.realloc = mem_wrap_realloc,
    setup.free = mem_wrap_free,
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));
    binrec_set_optimization_flags(handle, opt_flags, 0, 0);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    if (add_rtl(unit) != EXIT_SUCCESS) {
        const int line = __LINE__ - 1;
        const char *log_messages = get_log_messages();
        printf("%s:%d: add_rtl(unit) failed (%s)\n%s", __FILE__, line,
               log_messages ? "log follows" : "no errors logged",
               log_messages ? log_messages : "");
        return EXIT_FAILURE;
    }

    if (!rtl_finalize_unit(unit)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("rtl_finalize_unit(unit) was not true as expected");
    }

    unit->blocks_size = unit->num_blocks;
    mem_wrap_fail_after(1);
    if (!rtl_optimize_unit(unit, opt_flags)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("rtl_optimize_unit(unit, opt_flags) was not true as expected");
    }
    mem_wrap_cancel_fail();

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, false));

    char *output;
    const char *log_messages = get_log_messages();
    if (!log_messages) {
        log_messages = "";
    }
    const char *ice;
    while ((ice = strstr(log_messages, "Internal compiler error:")) != NULL) {
        log_messages = ice + strcspn(ice, "\n");
        ASSERT(*log_messages == '\n');
        log_messages++;
    }

    const int output_size = strlen(log_messages) + strlen(disassembly) + 1;
    ASSERT(output = malloc(output_size));
    ASSERT(snprintf(output, output_size, "%s%s", log_messages, disassembly)
           == output_size - 1);
    EXPECT_STREQ(output, expected);

    free(output);
    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
