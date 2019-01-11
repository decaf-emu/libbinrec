/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "src/endian.h"
#include "src/guest-ppc.h"
#include "src/rtl.h"
#include "src/rtl-internal.h"
#include "tests/common.h"
#include "tests/guest-ppc/common.h"
#include "tests/guest-ppc/insn/common.h"


int main(void)
{
    uint8_t *memory;
    EXPECT(memory = malloc(0x10000));

    static const uint32_t ppc_code[] = {
        0x38600001,  // li r3,1
        0x2F830001,  // cmpi cr7,r3,1
    };
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    binrec_setup_t final_setup;
    memcpy(&final_setup, &setup, sizeof(setup));
    final_setup.guest_memory_base = memory;
    binrec_t *handle;
    EXPECT(handle = binrec_create_handle(&final_setup));

    binrec_set_pre_insn_callback(handle, (void (*)(void *, uint32_t))1);

    RTLUnit *unit;
    EXPECT(unit = rtl_create_unit(handle));
    EXPECT(guest_ppc_translate(handle, 0x1000, 0x1007, unit));
    EXPECT_STREQ(rtl_disassemble_unit(unit, false),
                 "    0: LOAD_ARG   r1, 0\n"
                 "    1: LOAD_ARG   r2, 1\n"
                 "    2: LOAD_IMM   r3, 0x1\n"
                 "    3: LOAD_IMM   r4, 4096\n"
                 "    4: CALL_TRANSPARENT @r3, r1, r4\n"
                 "    5: LOAD_IMM   r5, 1\n"
                 "    6: SET_ALIAS  a2, r5\n"
                 "    7: LOAD_IMM   r6, 0x1\n"
                 "    8: LOAD_IMM   r7, 4100\n"
                 "    9: CALL_TRANSPARENT @r6, r1, r7\n"
                 "   10: SLTSI      r8, r5, 1\n"
                 "   11: SGTSI      r9, r5, 1\n"
                 "   12: SEQI       r10, r5, 1\n"
                 "   13: GET_ALIAS  r11, a4\n"
                 "   14: BFEXT      r12, r11, 31, 1\n"
                 "   15: GET_ALIAS  r13, a3\n"
                 "   16: SLLI       r14, r8, 3\n"
                 "   17: SLLI       r15, r9, 2\n"
                 "   18: SLLI       r16, r10, 1\n"
                 "   19: OR         r17, r14, r15\n"
                 "   20: OR         r18, r16, r12\n"
                 "   21: OR         r19, r17, r18\n"
                 "   22: BFINS      r20, r13, r19, 0, 4\n"
                 "   23: SET_ALIAS  a3, r20\n"
                 "   24: LOAD_IMM   r21, 4104\n"
                 "   25: SET_ALIAS  a1, r21\n"
                 "   26: RETURN     r1\n"
                 "\n"
                 "Alias 1: int32 @ 964(r1)\n"
                 "Alias 2: int32 @ 268(r1)\n"
                 "Alias 3: int32 @ 928(r1)\n"
                 "Alias 4: int32 @ 940(r1)\n"
                 "\n"
                 "Block 0: <none> --> [0,26] --> <none>\n"
                 );

    rtl_destroy_unit(unit);
    binrec_destroy_handle(handle);
    free(memory);
    return EXIT_SUCCESS;
}
