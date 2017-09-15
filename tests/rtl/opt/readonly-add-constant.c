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
    static uint32_t value_buf[2];
    value_buf[0] = 1234;
    value_buf[1] = 5678;
    /* Avoid spurious warnings. */
    #ifdef __GNUC__
        #pragma GCC diagnostic ignored "-Warray-bounds"
    #endif
    unit->handle->setup.guest_memory_base =
        (char *)value_buf - (uintptr_t)0xC0000000;
    unit->handle->host_little_endian = is_little_endian();
    binrec_add_readonly_region(unit->handle, 0xC0000000, 8);

    int reg1, reg2, reg3, reg4, reg5, reg6, reg7;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    rtl_set_membase_pointer(unit, reg1);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 0xC0000004));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg3, reg1, reg2, 0));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg4, reg3, 0, -4));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg5, 0, 0, 0xC0000000));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg6, reg5, reg1, 0));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg7, reg6, 0, 4));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] r4 loads constant value 1234 from 0xC0000000 at 3\n"
        "[info] r3 no longer used, setting death = birth\n"
        "[info] r7 loads constant value 5678 from 0xC0000004 at 6\n"
        "[info] r6 no longer used, setting death = birth\n"
    #endif
    "    0: LOAD_IMM   r1, 0x0\n"
    "    1: LOAD_IMM   r2, 0xC0000004\n"
    "    2: ADD        r3, r1, r2\n"
    "    3: LOAD_IMM   r4, 1234\n"
    "    4: LOAD_IMM   r5, 0xC0000000\n"
    "    5: ADD        r6, r5, r1\n"
    "    6: LOAD_IMM   r7, 5678\n"
    "\n"
    "Block 0: <none> --> [0,6] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
