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
    memory[READONLY_PAGE_SIZE*2-1] = 1;
    memory[READONLY_PAGE_SIZE*2] = 2;
    unit->handle->setup.guest_memory_base = memory;
    unit->handle->host_little_endian = is_little_endian();
    binrec_add_readonly_region(unit->handle, READONLY_PAGE_SIZE*2 - 1, 2);

    int reg1, reg2, reg3, reg4, reg5, reg6;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg1, 0, 0, 0));
    rtl_set_membase_pointer(unit, reg1);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg2, reg1, 0, READONLY_PAGE_SIZE*2));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_U8, reg3, reg2, 0, -2));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_U8, reg4, reg2, 0, -1));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_U8, reg5, reg2, 0, 0));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_U8, reg6, reg2, 0, 1));

    return EXIT_SUCCESS;
}

static const char expected[] =
    #ifdef RTL_DEBUG_OPTIMIZE
        "[info] r4 loads constant value 1 from 0x1FFF at 3\n"
        "[info] r5 loads constant value 2 from 0x2000 at 4\n"
    #endif
    "    0: LOAD_IMM   r1, 0x0\n"
    "    1: ADDI       r2, r1, 8192\n"
    "    2: LOAD_U8    r3, -2(r2)\n"
    "    3: LOAD_IMM   r4, 1\n"
    "    4: LOAD_IMM   r5, 2\n"
    "    5: LOAD_U8    r6, 1(r2)\n"
    "\n"
    "Block 0: <none> --> [0,5] --> <none>\n"
    ;

#include "tests/rtl-optimize-test.i"
