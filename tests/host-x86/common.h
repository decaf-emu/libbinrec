/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef TESTS_HOST_X86_COMMON_H
#define TESTS_HOST_X86_COMMON_H

#ifndef SRC_RTL_INTERNAL_H
    #include "src/rtl-internal.h"
#endif

/*************************************************************************/
/*************************************************************************/

/**
 * alloc_dummy_registers:  Allocate dummy RTL registers to occupy slots in
 * the host register map.
 *
 * [Parameters]
 *     unit: RTL unit on which to operate.
 *     count: Number of registers to allocate.
 *     type: Register type (RTLTYPE_*).
 * [Return value]
 *     True on success, false on error.
 */
static inline bool alloc_dummy_registers(RTLUnit *unit, int count,
                                         RTLDataType type)
{
    for (int i = 0; i < count; i++) {
        uint32_t reg;
        EXPECT(reg = rtl_alloc_register(unit, type));
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, reg, 0, 0, 0));
        /* Hide the register's death from the register allocator. */
        unit->regs[reg].death = unit->num_insns;
        EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 0));
    }
    return true;
}

/*************************************************************************/
/*************************************************************************/

#endif  // TESTS_HOST_X86_COMMON_H
