/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/log-capture.h"


int main(void)
{
    binrec_setup_t setup;
    memset(&setup, 0, sizeof(setup));
    setup.log = log_capture;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&setup));

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));

    const int NUM_BLOCKS = 1000;
    for (int i = 0; i < NUM_BLOCKS; i++) {
        EXPECT_EQ(rtl_alloc_label(unit), i+1);
        EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, i+1));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    }

    EXPECT(rtl_finalize_unit(unit));

    const char *disassembly;
    EXPECT(disassembly = rtl_disassemble_unit(unit, false));

    const char *s = disassembly;

    for (int i = 0; i < NUM_BLOCKS; i++) {
        char line[100], expect[100];
        const char *eol;

        eol = s + strcspn(s, "\n") + 1;
        EXPECT(snprintf(line, sizeof(line), "%.*s",
                        (int)(eol - s), s) < (int)sizeof(line));
        ASSERT(snprintf(expect, sizeof(expect), "%5d: LABEL      L%d\n",
                        i*2, i+1) < (int)sizeof(expect));
        EXPECT_STREQ(line, expect);
        s = eol;

        eol = s + strcspn(s, "\n") + 1;
        EXPECT(snprintf(line, sizeof(line),
                        "%.*s", (int)(eol - s), s) < (int)sizeof(line));
        ASSERT(snprintf(expect, sizeof(expect),
                        "%5d: NOP\n", i*2+1) < (int)sizeof(expect));
        EXPECT_STREQ(line, expect);
        s = eol;
    }

    EXPECT(*s++ == '\n');

    for (int i = 0; i < NUM_BLOCKS; i++) {
        char line[100], expect[100];
        const char *eol = s + strcspn(s, "\n") + 1;
        EXPECT(snprintf(line, sizeof(line),
                        "%.*s", (int)(eol - s), s) < (int)sizeof(line));
        if (i == 0) {
            ASSERT(snprintf(expect, sizeof(expect),
                            "Block %d: <none> --> [%d,%d] --> %d\n",
                            i, i*2, i*2+1, i+1) < (int)sizeof(expect));
        } else if (i == NUM_BLOCKS - 1) {
            ASSERT(snprintf(expect, sizeof(expect),
                            "Block %d: %d --> [%d,%d] --> <none>\n",
                            i, i-1, i*2, i*2+1) < (int)sizeof(expect));
        } else {
            ASSERT(snprintf(expect, sizeof(expect),
                            "Block %d: %d --> [%d,%d] --> %d\n",
                            i, i-1, i*2, i*2+1, i+1) < (int)sizeof(expect));
        }
        EXPECT_STREQ(line, expect);
        s = eol;
    }

    EXPECT(!*s);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
