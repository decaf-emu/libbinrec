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

    unit->aliases_size = ALIASES_LIMIT - 1;
    unit->next_alias = unit->aliases_size;
    EXPECT_EQ(rtl_alloc_alias_register(unit, RTLTYPE_INT32),
              ALIASES_LIMIT - 1);
    EXPECT_EQ(unit->aliases_size, ALIASES_LIMIT);
    EXPECT_EQ(unit->next_alias, ALIASES_LIMIT);
    EXPECT_EQ(unit->alias_types[ALIASES_LIMIT - 1], RTLTYPE_INT32);

    EXPECT_EQ(rtl_alloc_alias_register(unit, RTLTYPE_ADDRESS), 0);
    EXPECT_EQ(unit->aliases_size, ALIASES_LIMIT);
    EXPECT_EQ(unit->next_alias, ALIASES_LIMIT);
    EXPECT_EQ(unit->alias_types[ALIASES_LIMIT - 1], RTLTYPE_INT32);

    char expected_log[100];
    snprintf(expected_log, sizeof(expected_log),
             "[error] Too many alias registers in unit (limit %u)\n",
             ALIASES_LIMIT);
    EXPECT_STREQ(get_log_messages(), expected_log);

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    return EXIT_SUCCESS;
}
