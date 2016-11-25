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
    static uint64_t value_buf[3];
    memset(value_buf, 0x80, sizeof(value_buf));
    value_buf[1] = 0x123456789;
    unit->handle->setup.guest_memory_base = value_buf;
    unit->handle->host_little_endian = is_little_endian();
    binrec_add_readonly_region(unit->handle, 0, sizeof(value_buf));

    int reg1, reg2, reg3, reg4, reg5;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    rtl_set_membase_pointer(unit, reg1);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI,
                        reg2, reg1, 0, 2*sizeof(*value_buf)));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg3, reg2, 0, sizeof(*value_buf)));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ZCAST, reg4, reg3, 0, 0));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT64));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg5, reg4, 0, -sizeof(*value_buf)));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_IMM   r1, 0x0\n"
    "    1: ADDI       r2, r1, 16\n"
    "    2: LOAD       r3, 8(r2)\n"
    "    3: ZCAST      r4, r3\n"
    "    4: LOAD       r5, -8(r4)\n"
    "\n"
    "Block 0: <none> --> [0,4] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
