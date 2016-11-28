/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/bitutils.h"
#include "src/common.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"

#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

/*************************************************************************/
/************* Exported utility routines (library-internal) **************/
/*************************************************************************/

FORMAT(3, 4)
int snprintf_assert(char *buf, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buf, size, format, args);
    va_end(args);
    ASSERT((size_t)result < size);
    return result;
}

/*-----------------------------------------------------------------------*/

int format_int(char *buf, int bufsize, RTLDataType type, uint64_t value)
{
    switch (type) {
      case RTLTYPE_INT32:
        if ((uint32_t)value + 0x8000 >= 0x18000) {
            /* i.e., value is outside [-0x8000,+0xFFFF] */
            return snprintf_assert(buf, bufsize, "0x%X", (uint32_t)value);
        } else {
            return snprintf_assert(buf, bufsize, "%d", (int32_t)value);
        }
      case RTLTYPE_INT64:
        if (value + 0x8000 >= 0x18000) {
            return snprintf_assert(buf, bufsize, "0x%"PRIX64, value);
        } else {
            return snprintf_assert(buf, bufsize, "%d", (int32_t)value);
        }
      default:
        ASSERT(type == RTLTYPE_ADDRESS);
        return snprintf_assert(buf, bufsize, "0x%"PRIX64, value);
    }
}

/*-----------------------------------------------------------------------*/

int format_float(char *buf, int bufsize, float value)
{
    if (isnan(value)) {
        return snprintf_assert(buf, bufsize, "%snan(0x%X)",
                               float_to_bits(value)>>31 ? "-" : "",
                               float_to_bits(value) & 0x007FFFFF);
    } else if (isinf(value)) {
        return snprintf_assert(buf, bufsize, value < 0.0f ? "-inf" : "inf");
    } else {
        int n = snprintf_assert(buf, bufsize, "%.8g", value);
        if (!strchr(buf, '.')) {
            n += snprintf_assert(buf+n, bufsize-n, ".0");
        }
        n += snprintf_assert(buf+n, bufsize-n, "f");
        return n;
    }
}

/*-----------------------------------------------------------------------*/

int format_double(char *buf, int bufsize, double value)
{
    if (isnan(value)) {
        return snprintf_assert(
            buf, bufsize, "%snan(0x%"PRIX64")",
            double_to_bits(value)>>63 ? "-" : "",
            double_to_bits(value) & UINT64_C(0x000FFFFFFFFFFFFF));
    } else if (isinf(value)) {
        return snprintf_assert(buf, bufsize, value < 0.0 ? "-inf" : "inf");
    } else {
        int n = snprintf_assert(buf, bufsize, "%.17g", value);
        if (!strchr(buf, '.')) {
            n += snprintf_assert(buf+n, bufsize-n, ".0");
        }
        return n;
    }
}

/*-----------------------------------------------------------------------*/

const char *rtl_type_name(RTLDataType type)
{
    static const char * const names[] = {
        [RTLTYPE_INT32     ] = "int32",
        [RTLTYPE_INT64     ] = "int64",
        [RTLTYPE_ADDRESS   ] = "address",
        [RTLTYPE_FLOAT32   ] = "float32",
        [RTLTYPE_FLOAT64   ] = "float64",
        [RTLTYPE_V2_FLOAT32] = "float32[2]",
        [RTLTYPE_V2_FLOAT64] = "float64[2]",
        [RTLTYPE_FPSTATE   ] = "fpstate"
    };
    ASSERT(type > 0 && type < lenof(names));
    ASSERT(names[type]);
    return names[type];
}

/*-----------------------------------------------------------------------*/

const char *rtl_type_suffix(RTLDataType type)
{
    static const char * const suffixes[] = {
        [RTLTYPE_INT32     ] = "i32",
        [RTLTYPE_INT64     ] = "i64",
        [RTLTYPE_ADDRESS   ] = "addr",
        [RTLTYPE_FLOAT32   ] = "f32",
        [RTLTYPE_FLOAT64   ] = "f64",
        [RTLTYPE_V2_FLOAT32] = "f32x2",
        [RTLTYPE_V2_FLOAT64] = "f64x2",
        [RTLTYPE_FPSTATE   ] = "fpstate"
    };
    ASSERT(type > 0 && type < lenof(suffixes));
    ASSERT(suffixes[type]);
    return suffixes[type];
}

/*-----------------------------------------------------------------------*/

const char *rtl_source_name(RTLRegType source)
{
    static const char * const names[] = {
        [RTLREG_UNDEFINED      ] = "undefined",
        [RTLREG_CONSTANT       ] = "constant",
        [RTLREG_CONSTANT_NOFOLD] = "constant_unfoldable",
        [RTLREG_FUNC_ARG       ] = "argument",
        [RTLREG_MEMORY         ] = "memory",
        [RTLREG_ALIAS          ] = "alias",
        [RTLREG_RESULT         ] = "result",
        [RTLREG_RESULT_NOFOLD  ] = "result_unfoldable",
    };
    ASSERT((unsigned int)source < lenof(names));
    ASSERT(names[source]);
    return names[source];
}

/*-----------------------------------------------------------------------*/

void rtl_update_live_ranges(RTLUnit * const unit)
{
    ASSERT(unit != NULL);
    ASSERT(unit->insns != NULL);
    ASSERT(unit->blocks != NULL);
    ASSERT(unit->regs != NULL);

    for (int block_index = 0; block_index != -1;
         block_index = unit->blocks[block_index].next_block)
    {
        RTLBlock * const block = &unit->blocks[block_index];
        int latest_entry_block = -1;
        for (int entry_index = block_index; entry_index >= 0;
             entry_index = unit->blocks[entry_index].entry_overflow)
        {
            const RTLBlock * const entry_block = &unit->blocks[entry_index];
            for (int i = 0; (i < lenof(entry_block->entries)
                             && entry_block->entries[i] >= 0); i++) {
                if (entry_block->entries[i] > latest_entry_block) {
                    latest_entry_block = entry_block->entries[i];
                }
            }
        }
        if (latest_entry_block >= block_index) {
            const int birth_limit = block->first_insn;
            const int min_death = unit->blocks[latest_entry_block].last_insn;
            block->min_death = min_death;
            for (int reg = unit->first_live_reg; reg <= block->max_live_reg;
                 reg++)
            {
                if (unit->regs[reg].birth < birth_limit
                 && unit->regs[reg].death >= birth_limit
                 && unit->regs[reg].death < min_death) {
                    unit->regs[reg].death = min_death;
                }
            }
        }
    }
}

/*************************************************************************/
/*************************************************************************/
