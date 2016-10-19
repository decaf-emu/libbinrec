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


static unsigned int opt_flags = BINREC_OPT_BASIC;

static int add_rtl(RTLUnit *unit)
{
    int label1, label2;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_RETURN, 0, 0, 0, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO, 0, 0, 0, label1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "[warning] Out of memory while threading branch at 0\n"
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] Dropping dead block 8 (10-10)\n"
        "[info] Killing instruction 10\n"
        "[info] Dropping dead block 7 (9-9)\n"
        "[info] Killing instruction 9\n"
        "[info] Dropping dead block 6 (8-8)\n"
        "[info] Killing instruction 8\n"
        "[info] Dropping dead block 5 (7-7)\n"
        "[info] Killing instruction 7\n"
        "[info] Dropping dead block 4 (6-6)\n"
        "[info] Killing instruction 6\n"
        "[info] Dropping dead block 3 (5-5)\n"
        "[info] Killing instruction 5\n"
    #endif
    "    0: GOTO       L2\n"
    "    1: LABEL      L1\n"
    "    2: RETURN\n"
    "    3: LABEL      L2\n"
    "    4: GOTO       L1\n"
    "    5: NOP\n"
    "    6: NOP\n"
    "    7: NOP\n"
    "    8: NOP\n"
    "    9: NOP\n"
    "   10: NOP\n"
    "\n"
    "Block 0: <none> --> [0,0] --> 2\n"
    "Block 1: 2 --> [1,2] --> <none>\n"
    "Block 2: 0 --> [3,4] --> 1\n"
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
