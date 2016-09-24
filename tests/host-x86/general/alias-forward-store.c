/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "tests/common.h"
#include "tests/host-x86/common.h"


static const binrec_setup_t setup = {
    .host = BINREC_ARCH_X86_64_SYSV,
};
static const unsigned int host_opt = 0;

static int add_rtl(RTLUnit *unit)
{
    int reg1, reg2, reg3, alias1, alias2;
    EXPECT(reg1 = rtl_alloc_register(unit, RTLTYPE_ADDRESS));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_ARG, reg1, 0, 0, 0));
    EXPECT(alias1 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias1, reg1, 0x1234);
    EXPECT(alias2 = rtl_alloc_alias_register(unit, RTLTYPE_INT32));
    rtl_set_alias_storage(unit, alias2, reg1, 0x5678);
    EXPECT(reg2 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg2, 0, 0, 2));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg2, 0, alias1));
    EXPECT(reg3 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg3, 0, 0, 3));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg3, 0, alias2));

    int label1;
    EXPECT(label1 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label1));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 1));
    /* Stores should be forwarded through this block. */

    int label2, reg4, reg5;
    EXPECT(label2 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label2));
    EXPECT(reg4 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg4, 0, 0, alias2));
    EXPECT(reg5 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg5, reg4, 0, 5));
    /* Only alias1 should be forwarded, due to the load of alias2. */

    int label3, reg6, reg7, reg8, reg9;
    EXPECT(label3 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label3));
    EXPECT(reg6 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg6, 0, 0, alias1));
    EXPECT(reg7 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg7, 0, 0, alias2));
    EXPECT(reg8 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg8, reg6, 0, 8));
    EXPECT(reg9 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADDI, reg9, reg7, 0, 9));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg8, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg9, 0, alias2));

    int label4;
    EXPECT(label4 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label4));
    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, 0, 0, 4));
    /* Stores should be not forwarded through this block since it has
     * multiple entry edges. */

    int label5, reg10, reg11, reg12;
    EXPECT(label5 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label5));
    EXPECT(reg10 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg10, 0, 0, alias1));
    EXPECT(reg11 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg11, 0, 0, alias2));
    EXPECT(reg12 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg12, reg10, reg11, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg12, 0, alias1));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg11, 0, alias2));
    EXPECT(rtl_add_insn(unit, RTLOP_GOTO_IF_Z, 0, reg12, 0, label4));

    int label6, reg13;
    EXPECT(label6 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label6));
    EXPECT(reg13 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_LOAD_IMM, reg13, 0, 0, 13));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg13, 0, alias1));
    /* The store to alias1 should override store forwarding for that alias. */

    int label7, reg14, reg15, reg16;
    EXPECT(label7 = rtl_alloc_label(unit));
    EXPECT(rtl_add_insn(unit, RTLOP_LABEL, 0, 0, 0, label7));
    EXPECT(reg14 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg14, 0, 0, alias1));
    EXPECT(reg15 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_GET_ALIAS, reg15, 0, 0, alias2));
    EXPECT(reg16 = rtl_alloc_register(unit, RTLTYPE_INT32));
    EXPECT(rtl_add_insn(unit, RTLOP_ADD, reg16, reg14, reg15, 0));
    EXPECT(rtl_add_insn(unit, RTLOP_SET_ALIAS, 0, reg16, 0, alias1));

    EXPECT(rtl_add_insn(unit, RTLOP_NOP, 0, reg1, 0, 0));
    return EXIT_SUCCESS;
}

static const uint8_t expected_code[] = {
    0x48,0x83,0xEC,0x08,                // sub $8,%rsp

    /* Block 0 */
    0xB8,0x02,0x00,0x00,0x00,           // mov $2,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0xB9,0x03,0x00,0x00,0x00,           // mov $3,%ecx
    0x89,0x8F,0x78,0x56,0x00,0x00,      // mov %ecx,0x5678(%rdi)

    /* Block 1 */
    0x0F,0x1F,0x05,0x01,0x00,0x00,0x00, // nop 1(%rip)

    /* Block 2 */
    0x83,0xC1,0x05,                     // add $5,%ecx

    /* Block 3 */
    0x8B,0x8F,0x78,0x56,0x00,0x00,      // mov 0x5678(%rdi),%ecx
    0x83,0xC0,0x08,                     // add $8,%eax
    0x83,0xC1,0x09,                     // add $9,%ecx
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x89,0x8F,0x78,0x56,0x00,0x00,      // mov %ecx,0x5678(%rdi)

    /* Block 4 */
    0x0F,0x1F,0x05,0x04,0x00,0x00,0x00, // L4: nop 4(%rip)

    /* Block 5 */
    0x8B,0x87,0x34,0x12,0x00,0x00,      // mov 0x1234(%rdi),%eax
    0x8B,0x8F,0x78,0x56,0x00,0x00,      // mov 0x5678(%rdi),%ecx
    0x03,0xC1,                          // add %ecx,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)
    0x89,0x8F,0x78,0x56,0x00,0x00,      // mov %ecx,0x5678(%rdi)
    0x85,0xC0,                          // test %eax,%eax
    0x74,0xDB,                          // jz L4

    /* Block 6 */
    0xB8,0x0D,0x00,0x00,0x00,           // mov $13,%eax

    /* Block 7 */
    0x03,0xC1,                          // add %ecx,%eax
    0x89,0x87,0x34,0x12,0x00,0x00,      // mov %eax,0x1234(%rdi)

    0x48,0x83,0xC4,0x08,                // add $8,%rsp
    0xC3,                               // ret
};

static const char expected_log[] = "";

#include "tests/rtl-translate-test.i"
