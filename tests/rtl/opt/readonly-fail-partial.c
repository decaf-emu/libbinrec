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
    static uint8_t memory[READONLY_PAGE_SIZE*3];
    memset(memory, 0x80, sizeof(memory));
    unit->handle->setup.guest_memory_base = memory;
    unit->handle->host_little_endian = is_little_endian();
    binrec_add_readonly_region(unit->handle, 4, sizeof(memory)-4);

    int reg1, reg2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    rtl_set_membase_pointer(unit, reg1);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD, reg2, reg1, 0, 0));

    return EXIT_SUCCESS;
}

static const char expected[] =
    "    0: LOAD_IMM   r1, 0x0\n"
    "    1: LOAD       r2, 0(r1)\n"
    "\n"
    "Block 0: <none> --> [0,1] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
