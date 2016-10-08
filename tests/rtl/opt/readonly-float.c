/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/endian.h"
#include "src/rtl-internal.h"
#include "tests/common.h"


static unsigned int opt_flags = BINREC_OPT_FOLD_CONSTANTS;

static int add_rtl(RTLUnit *unit)
{
    static uint32_t value_buf[3];
    memset(value_buf, 0x80, sizeof(value_buf));
    value_buf[1] = 0x3F800000;
    unit->handle->setup.guest_memory_base = value_buf;
    unit->handle->host_little_endian = is_little_endian();
    binrec_add_readonly_region(unit->handle, 0, sizeof(value_buf));

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM,
                        reg1, 0, 0, 2*sizeof(*value_buf)));
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_FLOAT));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, -sizeof(*value_buf)));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] r2 loads constant value 0x3F800000 from 0x4 at insn 1\n"
        "[info] r1 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 0x8\n"
    "    1: LOAD_IMM   r2, 1.0f\n"
    "\n"
    "Block 0: <none> --> [0,1] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
