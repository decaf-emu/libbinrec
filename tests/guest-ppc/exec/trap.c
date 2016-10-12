/*
 * libbinrec: a recompiling translator for machine code
 * Copyright (c) 2016 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/binrec.h"
#include "src/endian.h"
#include "tests/common.h"
#include "tests/execute.h"
#include "tests/guest-ppc/common.h"
#include "tests/log-capture.h"


static const uint32_t ppc_code[] = {
    0x3860FFF6,  // li r3,-10
    0x3880000A,  // li r4,10

    0x0C03FFEC,  // twi 0,r3,-20
    0x0C03FFF6,  // twi 0,r3,-10
    0x0C03FFFB,  // twi 0,r3,-5
    0x0C030005,  // twi 0,r3,5
    0x0C03000A,  // twi 0,r3,10
    0x0C030014,  // twi 0,r3,20
    0x0C04FFEC,  // twi 0,r4,-20
    0x0C04FFF6,  // twi 0,r4,-10
    0x0C04FFFB,  // twi 0,r4,-5
    0x0C040005,  // twi 0,r4,5
    0x0C04000A,  // twi 0,r4,10
    0x0C040014,  // twi 0,r4,20
    0x0C23FFEC,  // twi 1,r3,-20
    0x0C23FFF6,  // twi 1,r3,-10
    0x0C23FFFB,  // twi 1,r3,-5
    0x0C230005,  // twi 1,r3,5
    0x0C23000A,  // twi 1,r3,10
    0x0C230014,  // twi 1,r3,20
    0x0C24FFEC,  // twi 1,r4,-20
    0x0C24FFF6,  // twi 1,r4,-10
    0x0C24FFFB,  // twi 1,r4,-5
    0x0C240005,  // twi 1,r4,5
    0x0C24000A,  // twi 1,r4,10
    0x0C240014,  // twi 1,r4,20
    0x0C43FFEC,  // twi 2,r3,-20
    0x0C43FFF6,  // twi 2,r3,-10
    0x0C43FFFB,  // twi 2,r3,-5
    0x0C430005,  // twi 2,r3,5
    0x0C43000A,  // twi 2,r3,10
    0x0C430014,  // twi 2,r3,20
    0x0C44FFEC,  // twi 2,r4,-20
    0x0C44FFF6,  // twi 2,r4,-10
    0x0C44FFFB,  // twi 2,r4,-5
    0x0C440005,  // twi 2,r4,5
    0x0C44000A,  // twi 2,r4,10
    0x0C440014,  // twi 2,r4,20
    0x0C63FFEC,  // twi 3,r3,-20
    0x0C63FFF6,  // twi 3,r3,-10
    0x0C63FFFB,  // twi 3,r3,-5
    0x0C630005,  // twi 3,r3,5
    0x0C63000A,  // twi 3,r3,10
    0x0C630014,  // twi 3,r3,20
    0x0C64FFEC,  // twi 3,r4,-20
    0x0C64FFF6,  // twi 3,r4,-10
    0x0C64FFFB,  // twi 3,r4,-5
    0x0C640005,  // twi 3,r4,5
    0x0C64000A,  // twi 3,r4,10
    0x0C640014,  // twi 3,r4,20
    0x0C83FFEC,  // twi 4,r3,-20
    0x0C83FFF6,  // twi 4,r3,-10
    0x0C83FFFB,  // twi 4,r3,-5
    0x0C830005,  // twi 4,r3,5
    0x0C83000A,  // twi 4,r3,10
    0x0C830014,  // twi 4,r3,20
    0x0C84FFEC,  // twi 4,r4,-20
    0x0C84FFF6,  // twi 4,r4,-10
    0x0C84FFFB,  // twi 4,r4,-5
    0x0C840005,  // twi 4,r4,5
    0x0C84000A,  // twi 4,r4,10
    0x0C840014,  // twi 4,r4,20
    0x0CA3FFEC,  // twi 5,r3,-20
    0x0CA3FFF6,  // twi 5,r3,-10
    0x0CA3FFFB,  // twi 5,r3,-5
    0x0CA30005,  // twi 5,r3,5
    0x0CA3000A,  // twi 5,r3,10
    0x0CA30014,  // twi 5,r3,20
    0x0CA4FFEC,  // twi 5,r4,-20
    0x0CA4FFF6,  // twi 5,r4,-10
    0x0CA4FFFB,  // twi 5,r4,-5
    0x0CA40005,  // twi 5,r4,5
    0x0CA4000A,  // twi 5,r4,10
    0x0CA40014,  // twi 5,r4,20
    0x0CC3FFEC,  // twi 6,r3,-20
    0x0CC3FFF6,  // twi 6,r3,-10
    0x0CC3FFFB,  // twi 6,r3,-5
    0x0CC30005,  // twi 6,r3,5
    0x0CC3000A,  // twi 6,r3,10
    0x0CC30014,  // twi 6,r3,20
    0x0CC4FFEC,  // twi 6,r4,-20
    0x0CC4FFF6,  // twi 6,r4,-10
    0x0CC4FFFB,  // twi 6,r4,-5
    0x0CC40005,  // twi 6,r4,5
    0x0CC4000A,  // twi 6,r4,10
    0x0CC40014,  // twi 6,r4,20
    0x0CE3FFEC,  // twi 7,r3,-20
    0x0CE3FFF6,  // twi 7,r3,-10
    0x0CE3FFFB,  // twi 7,r3,-5
    0x0CE30005,  // twi 7,r3,5
    0x0CE3000A,  // twi 7,r3,10
    0x0CE30014,  // twi 7,r3,20
    0x0CE4FFEC,  // twi 7,r4,-20
    0x0CE4FFF6,  // twi 7,r4,-10
    0x0CE4FFFB,  // twi 7,r4,-5
    0x0CE40005,  // twi 7,r4,5
    0x0CE4000A,  // twi 7,r4,10
    0x0CE40014,  // twi 7,r4,20
    0x0D03FFEC,  // twi 8,r3,-20
    0x0D03FFF6,  // twi 8,r3,-10
    0x0D03FFFB,  // twi 8,r3,-5
    0x0D030005,  // twi 8,r3,5
    0x0D03000A,  // twi 8,r3,10
    0x0D030014,  // twi 8,r3,20
    0x0D04FFEC,  // twi 8,r4,-20
    0x0D04FFF6,  // twi 8,r4,-10
    0x0D04FFFB,  // twi 8,r4,-5
    0x0D040005,  // twi 8,r4,5
    0x0D04000A,  // twi 8,r4,10
    0x0D040014,  // twi 8,r4,20
    0x0D23FFEC,  // twi 9,r3,-20
    0x0D23FFF6,  // twi 9,r3,-10
    0x0D23FFFB,  // twi 9,r3,-5
    0x0D230005,  // twi 9,r3,5
    0x0D23000A,  // twi 9,r3,10
    0x0D230014,  // twi 9,r3,20
    0x0D24FFEC,  // twi 9,r4,-20
    0x0D24FFF6,  // twi 9,r4,-10
    0x0D24FFFB,  // twi 9,r4,-5
    0x0D240005,  // twi 9,r4,5
    0x0D24000A,  // twi 9,r4,10
    0x0D240014,  // twi 9,r4,20
    0x0D43FFEC,  // twi 10,r3,-20
    0x0D43FFF6,  // twi 10,r3,-10
    0x0D43FFFB,  // twi 10,r3,-5
    0x0D430005,  // twi 10,r3,5
    0x0D43000A,  // twi 10,r3,10
    0x0D430014,  // twi 10,r3,20
    0x0D44FFEC,  // twi 10,r4,-20
    0x0D44FFF6,  // twi 10,r4,-10
    0x0D44FFFB,  // twi 10,r4,-5
    0x0D440005,  // twi 10,r4,5
    0x0D44000A,  // twi 10,r4,10
    0x0D440014,  // twi 10,r4,20
    0x0D63FFEC,  // twi 11,r3,-20
    0x0D63FFF6,  // twi 11,r3,-10
    0x0D63FFFB,  // twi 11,r3,-5
    0x0D630005,  // twi 11,r3,5
    0x0D63000A,  // twi 11,r3,10
    0x0D630014,  // twi 11,r3,20
    0x0D64FFEC,  // twi 11,r4,-20
    0x0D64FFF6,  // twi 11,r4,-10
    0x0D64FFFB,  // twi 11,r4,-5
    0x0D640005,  // twi 11,r4,5
    0x0D64000A,  // twi 11,r4,10
    0x0D640014,  // twi 11,r4,20
    0x0D83FFEC,  // twi 12,r3,-20
    0x0D83FFF6,  // twi 12,r3,-10
    0x0D83FFFB,  // twi 12,r3,-5
    0x0D830005,  // twi 12,r3,5
    0x0D83000A,  // twi 12,r3,10
    0x0D830014,  // twi 12,r3,20
    0x0D84FFEC,  // twi 12,r4,-20
    0x0D84FFF6,  // twi 12,r4,-10
    0x0D84FFFB,  // twi 12,r4,-5
    0x0D840005,  // twi 12,r4,5
    0x0D84000A,  // twi 12,r4,10
    0x0D840014,  // twi 12,r4,20
    0x0DA3FFEC,  // twi 13,r3,-20
    0x0DA3FFF6,  // twi 13,r3,-10
    0x0DA3FFFB,  // twi 13,r3,-5
    0x0DA30005,  // twi 13,r3,5
    0x0DA3000A,  // twi 13,r3,10
    0x0DA30014,  // twi 13,r3,20
    0x0DA4FFEC,  // twi 13,r4,-20
    0x0DA4FFF6,  // twi 13,r4,-10
    0x0DA4FFFB,  // twi 13,r4,-5
    0x0DA40005,  // twi 13,r4,5
    0x0DA4000A,  // twi 13,r4,10
    0x0DA40014,  // twi 13,r4,20
    0x0DC3FFEC,  // twi 14,r3,-20
    0x0DC3FFF6,  // twi 14,r3,-10
    0x0DC3FFFB,  // twi 14,r3,-5
    0x0DC30005,  // twi 14,r3,5
    0x0DC3000A,  // twi 14,r3,10
    0x0DC30014,  // twi 14,r3,20
    0x0DC4FFEC,  // twi 14,r4,-20
    0x0DC4FFF6,  // twi 14,r4,-10
    0x0DC4FFFB,  // twi 14,r4,-5
    0x0DC40005,  // twi 14,r4,5
    0x0DC4000A,  // twi 14,r4,10
    0x0DC40014,  // twi 14,r4,20
    0x0DE3FFEC,  // twi 15,r3,-20
    0x0DE3FFF6,  // twi 15,r3,-10
    0x0DE3FFFB,  // twi 15,r3,-5
    0x0DE30005,  // twi 15,r3,5
    0x0DE3000A,  // twi 15,r3,10
    0x0DE30014,  // twi 15,r3,20
    0x0DE4FFEC,  // twi 15,r4,-20
    0x0DE4FFF6,  // twi 15,r4,-10
    0x0DE4FFFB,  // twi 15,r4,-5
    0x0DE40005,  // twi 15,r4,5
    0x0DE4000A,  // twi 15,r4,10
    0x0DE40014,  // twi 15,r4,20
    0x0E03FFEC,  // twi 16,r3,-20
    0x0E03FFF6,  // twi 16,r3,-10
    0x0E03FFFB,  // twi 16,r3,-5
    0x0E030005,  // twi 16,r3,5
    0x0E03000A,  // twi 16,r3,10
    0x0E030014,  // twi 16,r3,20
    0x0E04FFEC,  // twi 16,r4,-20
    0x0E04FFF6,  // twi 16,r4,-10
    0x0E04FFFB,  // twi 16,r4,-5
    0x0E040005,  // twi 16,r4,5
    0x0E04000A,  // twi 16,r4,10
    0x0E040014,  // twi 16,r4,20
    0x0E23FFEC,  // twi 17,r3,-20
    0x0E23FFF6,  // twi 17,r3,-10
    0x0E23FFFB,  // twi 17,r3,-5
    0x0E230005,  // twi 17,r3,5
    0x0E23000A,  // twi 17,r3,10
    0x0E230014,  // twi 17,r3,20
    0x0E24FFEC,  // twi 17,r4,-20
    0x0E24FFF6,  // twi 17,r4,-10
    0x0E24FFFB,  // twi 17,r4,-5
    0x0E240005,  // twi 17,r4,5
    0x0E24000A,  // twi 17,r4,10
    0x0E240014,  // twi 17,r4,20
    0x0E43FFEC,  // twi 18,r3,-20
    0x0E43FFF6,  // twi 18,r3,-10
    0x0E43FFFB,  // twi 18,r3,-5
    0x0E430005,  // twi 18,r3,5
    0x0E43000A,  // twi 18,r3,10
    0x0E430014,  // twi 18,r3,20
    0x0E44FFEC,  // twi 18,r4,-20
    0x0E44FFF6,  // twi 18,r4,-10
    0x0E44FFFB,  // twi 18,r4,-5
    0x0E440005,  // twi 18,r4,5
    0x0E44000A,  // twi 18,r4,10
    0x0E440014,  // twi 18,r4,20
    0x0E63FFEC,  // twi 19,r3,-20
    0x0E63FFF6,  // twi 19,r3,-10
    0x0E63FFFB,  // twi 19,r3,-5
    0x0E630005,  // twi 19,r3,5
    0x0E63000A,  // twi 19,r3,10
    0x0E630014,  // twi 19,r3,20
    0x0E64FFEC,  // twi 19,r4,-20
    0x0E64FFF6,  // twi 19,r4,-10
    0x0E64FFFB,  // twi 19,r4,-5
    0x0E640005,  // twi 19,r4,5
    0x0E64000A,  // twi 19,r4,10
    0x0E640014,  // twi 19,r4,20
    0x0E83FFEC,  // twi 20,r3,-20
    0x0E83FFF6,  // twi 20,r3,-10
    0x0E83FFFB,  // twi 20,r3,-5
    0x0E830005,  // twi 20,r3,5
    0x0E83000A,  // twi 20,r3,10
    0x0E830014,  // twi 20,r3,20
    0x0E84FFEC,  // twi 20,r4,-20
    0x0E84FFF6,  // twi 20,r4,-10
    0x0E84FFFB,  // twi 20,r4,-5
    0x0E840005,  // twi 20,r4,5
    0x0E84000A,  // twi 20,r4,10
    0x0E840014,  // twi 20,r4,20
    0x0EA3FFEC,  // twi 21,r3,-20
    0x0EA3FFF6,  // twi 21,r3,-10
    0x0EA3FFFB,  // twi 21,r3,-5
    0x0EA30005,  // twi 21,r3,5
    0x0EA3000A,  // twi 21,r3,10
    0x0EA30014,  // twi 21,r3,20
    0x0EA4FFEC,  // twi 21,r4,-20
    0x0EA4FFF6,  // twi 21,r4,-10
    0x0EA4FFFB,  // twi 21,r4,-5
    0x0EA40005,  // twi 21,r4,5
    0x0EA4000A,  // twi 21,r4,10
    0x0EA40014,  // twi 21,r4,20
    0x0EC3FFEC,  // twi 22,r3,-20
    0x0EC3FFF6,  // twi 22,r3,-10
    0x0EC3FFFB,  // twi 22,r3,-5
    0x0EC30005,  // twi 22,r3,5
    0x0EC3000A,  // twi 22,r3,10
    0x0EC30014,  // twi 22,r3,20
    0x0EC4FFEC,  // twi 22,r4,-20
    0x0EC4FFF6,  // twi 22,r4,-10
    0x0EC4FFFB,  // twi 22,r4,-5
    0x0EC40005,  // twi 22,r4,5
    0x0EC4000A,  // twi 22,r4,10
    0x0EC40014,  // twi 22,r4,20
    0x0EE3FFEC,  // twi 23,r3,-20
    0x0EE3FFF6,  // twi 23,r3,-10
    0x0EE3FFFB,  // twi 23,r3,-5
    0x0EE30005,  // twi 23,r3,5
    0x0EE3000A,  // twi 23,r3,10
    0x0EE30014,  // twi 23,r3,20
    0x0EE4FFEC,  // twi 23,r4,-20
    0x0EE4FFF6,  // twi 23,r4,-10
    0x0EE4FFFB,  // twi 23,r4,-5
    0x0EE40005,  // twi 23,r4,5
    0x0EE4000A,  // twi 23,r4,10
    0x0EE40014,  // twi 23,r4,20
    0x0F03FFEC,  // twi 24,r3,-20
    0x0F03FFF6,  // twi 24,r3,-10
    0x0F03FFFB,  // twi 24,r3,-5
    0x0F030005,  // twi 24,r3,5
    0x0F03000A,  // twi 24,r3,10
    0x0F030014,  // twi 24,r3,20
    0x0F04FFEC,  // twi 24,r4,-20
    0x0F04FFF6,  // twi 24,r4,-10
    0x0F04FFFB,  // twi 24,r4,-5
    0x0F040005,  // twi 24,r4,5
    0x0F04000A,  // twi 24,r4,10
    0x0F040014,  // twi 24,r4,20
    0x0F23FFEC,  // twi 25,r3,-20
    0x0F23FFF6,  // twi 25,r3,-10
    0x0F23FFFB,  // twi 25,r3,-5
    0x0F230005,  // twi 25,r3,5
    0x0F23000A,  // twi 25,r3,10
    0x0F230014,  // twi 25,r3,20
    0x0F24FFEC,  // twi 25,r4,-20
    0x0F24FFF6,  // twi 25,r4,-10
    0x0F24FFFB,  // twi 25,r4,-5
    0x0F240005,  // twi 25,r4,5
    0x0F24000A,  // twi 25,r4,10
    0x0F240014,  // twi 25,r4,20
    0x0F43FFEC,  // twi 26,r3,-20
    0x0F43FFF6,  // twi 26,r3,-10
    0x0F43FFFB,  // twi 26,r3,-5
    0x0F430005,  // twi 26,r3,5
    0x0F43000A,  // twi 26,r3,10
    0x0F430014,  // twi 26,r3,20
    0x0F44FFEC,  // twi 26,r4,-20
    0x0F44FFF6,  // twi 26,r4,-10
    0x0F44FFFB,  // twi 26,r4,-5
    0x0F440005,  // twi 26,r4,5
    0x0F44000A,  // twi 26,r4,10
    0x0F440014,  // twi 26,r4,20
    0x0F63FFEC,  // twi 27,r3,-20
    0x0F63FFF6,  // twi 27,r3,-10
    0x0F63FFFB,  // twi 27,r3,-5
    0x0F630005,  // twi 27,r3,5
    0x0F63000A,  // twi 27,r3,10
    0x0F630014,  // twi 27,r3,20
    0x0F64FFEC,  // twi 27,r4,-20
    0x0F64FFF6,  // twi 27,r4,-10
    0x0F64FFFB,  // twi 27,r4,-5
    0x0F640005,  // twi 27,r4,5
    0x0F64000A,  // twi 27,r4,10
    0x0F640014,  // twi 27,r4,20
    0x0F83FFEC,  // twi 28,r3,-20
    0x0F83FFF6,  // twi 28,r3,-10
    0x0F83FFFB,  // twi 28,r3,-5
    0x0F830005,  // twi 28,r3,5
    0x0F83000A,  // twi 28,r3,10
    0x0F830014,  // twi 28,r3,20
    0x0F84FFEC,  // twi 28,r4,-20
    0x0F84FFF6,  // twi 28,r4,-10
    0x0F84FFFB,  // twi 28,r4,-5
    0x0F840005,  // twi 28,r4,5
    0x0F84000A,  // twi 28,r4,10
    0x0F840014,  // twi 28,r4,20
    0x0FA3FFEC,  // twi 29,r3,-20
    0x0FA3FFF6,  // twi 29,r3,-10
    0x0FA3FFFB,  // twi 29,r3,-5
    0x0FA30005,  // twi 29,r3,5
    0x0FA3000A,  // twi 29,r3,10
    0x0FA30014,  // twi 29,r3,20
    0x0FA4FFEC,  // twi 29,r4,-20
    0x0FA4FFF6,  // twi 29,r4,-10
    0x0FA4FFFB,  // twi 29,r4,-5
    0x0FA40005,  // twi 29,r4,5
    0x0FA4000A,  // twi 29,r4,10
    0x0FA40014,  // twi 29,r4,20
    0x0FC3FFEC,  // twi 30,r3,-20
    0x0FC3FFF6,  // twi 30,r3,-10
    0x0FC3FFFB,  // twi 30,r3,-5
    0x0FC30005,  // twi 30,r3,5
    0x0FC3000A,  // twi 30,r3,10
    0x0FC30014,  // twi 30,r3,20
    0x0FC4FFEC,  // twi 30,r4,-20
    0x0FC4FFF6,  // twi 30,r4,-10
    0x0FC4FFFB,  // twi 30,r4,-5
    0x0FC40005,  // twi 30,r4,5
    0x0FC4000A,  // twi 30,r4,10
    0x0FC40014,  // twi 30,r4,20
    0x0FE3FFEC,  // twi 31,r3,-20
    0x0FE3FFF6,  // twi 31,r3,-10
    0x0FE3FFFB,  // twi 31,r3,-5
    0x0FE30005,  // twi 31,r3,5
    0x0FE3000A,  // twi 31,r3,10
    0x0FE30014,  // twi 31,r3,20
    0x0FE4FFEC,  // twi 31,r4,-20
    0x0FE4FFF6,  // twi 31,r4,-10
    0x0FE4FFFB,  // twi 31,r4,-5
    0x0FE40005,  // twi 31,r4,5
    0x0FE4000A,  // twi 31,r4,10
    0x0FE40014,  // twi 31,r4,20

    0x4E800020,  // blr
};

static uint8_t *memory;

static int trap_count;  // Number of traps seen so far.
static uint32_t trap_insns[lenof(ppc_code)];  // List of trap-causing insns.
static const uint32_t expected_traps[] = {
    0x0C23FFEC,  // twi 1,r3,-20
    0x0C230005,  // twi 1,r3,5
    0x0C23000A,  // twi 1,r3,10
    0x0C230014,  // twi 1,r3,20
    0x0C240005,  // twi 1,r4,5
    0x0C43FFFB,  // twi 2,r3,-5
    0x0C44FFEC,  // twi 2,r4,-20
    0x0C44FFF6,  // twi 2,r4,-10
    0x0C44FFFB,  // twi 2,r4,-5
    0x0C440014,  // twi 2,r4,20
    0x0C63FFEC,  // twi 3,r3,-20
    0x0C63FFFB,  // twi 3,r3,-5
    0x0C630005,  // twi 3,r3,5
    0x0C63000A,  // twi 3,r3,10
    0x0C630014,  // twi 3,r3,20
    0x0C64FFEC,  // twi 3,r4,-20
    0x0C64FFF6,  // twi 3,r4,-10
    0x0C64FFFB,  // twi 3,r4,-5
    0x0C640005,  // twi 3,r4,5
    0x0C640014,  // twi 3,r4,20
    0x0C83FFF6,  // twi 4,r3,-10
    0x0C84000A,  // twi 4,r4,10
    0x0CA3FFEC,  // twi 5,r3,-20
    0x0CA3FFF6,  // twi 5,r3,-10
    0x0CA30005,  // twi 5,r3,5
    0x0CA3000A,  // twi 5,r3,10
    0x0CA30014,  // twi 5,r3,20
    0x0CA40005,  // twi 5,r4,5
    0x0CA4000A,  // twi 5,r4,10
    0x0CC3FFF6,  // twi 6,r3,-10
    0x0CC3FFFB,  // twi 6,r3,-5
    0x0CC4FFEC,  // twi 6,r4,-20
    0x0CC4FFF6,  // twi 6,r4,-10
    0x0CC4FFFB,  // twi 6,r4,-5
    0x0CC4000A,  // twi 6,r4,10
    0x0CC40014,  // twi 6,r4,20
    0x0CE3FFEC,  // twi 7,r3,-20
    0x0CE3FFF6,  // twi 7,r3,-10
    0x0CE3FFFB,  // twi 7,r3,-5
    0x0CE30005,  // twi 7,r3,5
    0x0CE3000A,  // twi 7,r3,10
    0x0CE30014,  // twi 7,r3,20
    0x0CE4FFEC,  // twi 7,r4,-20
    0x0CE4FFF6,  // twi 7,r4,-10
    0x0CE4FFFB,  // twi 7,r4,-5
    0x0CE40005,  // twi 7,r4,5
    0x0CE4000A,  // twi 7,r4,10
    0x0CE40014,  // twi 7,r4,20
    0x0D03FFEC,  // twi 8,r3,-20
    0x0D04FFEC,  // twi 8,r4,-20
    0x0D04FFF6,  // twi 8,r4,-10
    0x0D04FFFB,  // twi 8,r4,-5
    0x0D040005,  // twi 8,r4,5
    0x0D23FFEC,  // twi 9,r3,-20
    0x0D230005,  // twi 9,r3,5
    0x0D23000A,  // twi 9,r3,10
    0x0D230014,  // twi 9,r3,20
    0x0D24FFEC,  // twi 9,r4,-20
    0x0D24FFF6,  // twi 9,r4,-10
    0x0D24FFFB,  // twi 9,r4,-5
    0x0D240005,  // twi 9,r4,5
    0x0D43FFEC,  // twi 10,r3,-20
    0x0D43FFFB,  // twi 10,r3,-5
    0x0D44FFEC,  // twi 10,r4,-20
    0x0D44FFF6,  // twi 10,r4,-10
    0x0D44FFFB,  // twi 10,r4,-5
    0x0D440005,  // twi 10,r4,5
    0x0D440014,  // twi 10,r4,20
    0x0D63FFEC,  // twi 11,r3,-20
    0x0D63FFFB,  // twi 11,r3,-5
    0x0D630005,  // twi 11,r3,5
    0x0D63000A,  // twi 11,r3,10
    0x0D630014,  // twi 11,r3,20
    0x0D64FFEC,  // twi 11,r4,-20
    0x0D64FFF6,  // twi 11,r4,-10
    0x0D64FFFB,  // twi 11,r4,-5
    0x0D640005,  // twi 11,r4,5
    0x0D640014,  // twi 11,r4,20
    0x0D83FFEC,  // twi 12,r3,-20
    0x0D83FFF6,  // twi 12,r3,-10
    0x0D84FFEC,  // twi 12,r4,-20
    0x0D84FFF6,  // twi 12,r4,-10
    0x0D84FFFB,  // twi 12,r4,-5
    0x0D840005,  // twi 12,r4,5
    0x0D84000A,  // twi 12,r4,10
    0x0DA3FFEC,  // twi 13,r3,-20
    0x0DA3FFF6,  // twi 13,r3,-10
    0x0DA30005,  // twi 13,r3,5
    0x0DA3000A,  // twi 13,r3,10
    0x0DA30014,  // twi 13,r3,20
    0x0DA4FFEC,  // twi 13,r4,-20
    0x0DA4FFF6,  // twi 13,r4,-10
    0x0DA4FFFB,  // twi 13,r4,-5
    0x0DA40005,  // twi 13,r4,5
    0x0DA4000A,  // twi 13,r4,10
    0x0DC3FFEC,  // twi 14,r3,-20
    0x0DC3FFF6,  // twi 14,r3,-10
    0x0DC3FFFB,  // twi 14,r3,-5
    0x0DC4FFEC,  // twi 14,r4,-20
    0x0DC4FFF6,  // twi 14,r4,-10
    0x0DC4FFFB,  // twi 14,r4,-5
    0x0DC40005,  // twi 14,r4,5
    0x0DC4000A,  // twi 14,r4,10
    0x0DC40014,  // twi 14,r4,20
    0x0DE3FFEC,  // twi 15,r3,-20
    0x0DE3FFF6,  // twi 15,r3,-10
    0x0DE3FFFB,  // twi 15,r3,-5
    0x0DE30005,  // twi 15,r3,5
    0x0DE3000A,  // twi 15,r3,10
    0x0DE30014,  // twi 15,r3,20
    0x0DE4FFEC,  // twi 15,r4,-20
    0x0DE4FFF6,  // twi 15,r4,-10
    0x0DE4FFFB,  // twi 15,r4,-5
    0x0DE40005,  // twi 15,r4,5
    0x0DE4000A,  // twi 15,r4,10
    0x0DE40014,  // twi 15,r4,20
    0x0E03FFFB,  // twi 16,r3,-5
    0x0E030005,  // twi 16,r3,5
    0x0E03000A,  // twi 16,r3,10
    0x0E030014,  // twi 16,r3,20
    0x0E040014,  // twi 16,r4,20
    0x0E23FFEC,  // twi 17,r3,-20
    0x0E23FFFB,  // twi 17,r3,-5
    0x0E230005,  // twi 17,r3,5
    0x0E23000A,  // twi 17,r3,10
    0x0E230014,  // twi 17,r3,20
    0x0E240005,  // twi 17,r4,5
    0x0E240014,  // twi 17,r4,20
    0x0E43FFFB,  // twi 18,r3,-5
    0x0E430005,  // twi 18,r3,5
    0x0E43000A,  // twi 18,r3,10
    0x0E430014,  // twi 18,r3,20
    0x0E44FFEC,  // twi 18,r4,-20
    0x0E44FFF6,  // twi 18,r4,-10
    0x0E44FFFB,  // twi 18,r4,-5
    0x0E440014,  // twi 18,r4,20
    0x0E63FFEC,  // twi 19,r3,-20
    0x0E63FFFB,  // twi 19,r3,-5
    0x0E630005,  // twi 19,r3,5
    0x0E63000A,  // twi 19,r3,10
    0x0E630014,  // twi 19,r3,20
    0x0E64FFEC,  // twi 19,r4,-20
    0x0E64FFF6,  // twi 19,r4,-10
    0x0E64FFFB,  // twi 19,r4,-5
    0x0E640005,  // twi 19,r4,5
    0x0E640014,  // twi 19,r4,20
    0x0E83FFF6,  // twi 20,r3,-10
    0x0E83FFFB,  // twi 20,r3,-5
    0x0E830005,  // twi 20,r3,5
    0x0E83000A,  // twi 20,r3,10
    0x0E830014,  // twi 20,r3,20
    0x0E84000A,  // twi 20,r4,10
    0x0E840014,  // twi 20,r4,20
    0x0EA3FFEC,  // twi 21,r3,-20
    0x0EA3FFF6,  // twi 21,r3,-10
    0x0EA3FFFB,  // twi 21,r3,-5
    0x0EA30005,  // twi 21,r3,5
    0x0EA3000A,  // twi 21,r3,10
    0x0EA30014,  // twi 21,r3,20
    0x0EA40005,  // twi 21,r4,5
    0x0EA4000A,  // twi 21,r4,10
    0x0EA40014,  // twi 21,r4,20
    0x0EC3FFF6,  // twi 22,r3,-10
    0x0EC3FFFB,  // twi 22,r3,-5
    0x0EC30005,  // twi 22,r3,5
    0x0EC3000A,  // twi 22,r3,10
    0x0EC30014,  // twi 22,r3,20
    0x0EC4FFEC,  // twi 22,r4,-20
    0x0EC4FFF6,  // twi 22,r4,-10
    0x0EC4FFFB,  // twi 22,r4,-5
    0x0EC4000A,  // twi 22,r4,10
    0x0EC40014,  // twi 22,r4,20
    0x0EE3FFEC,  // twi 23,r3,-20
    0x0EE3FFF6,  // twi 23,r3,-10
    0x0EE3FFFB,  // twi 23,r3,-5
    0x0EE30005,  // twi 23,r3,5
    0x0EE3000A,  // twi 23,r3,10
    0x0EE30014,  // twi 23,r3,20
    0x0EE4FFEC,  // twi 23,r4,-20
    0x0EE4FFF6,  // twi 23,r4,-10
    0x0EE4FFFB,  // twi 23,r4,-5
    0x0EE40005,  // twi 23,r4,5
    0x0EE4000A,  // twi 23,r4,10
    0x0EE40014,  // twi 23,r4,20
    0x0F03FFEC,  // twi 24,r3,-20
    0x0F03FFFB,  // twi 24,r3,-5
    0x0F030005,  // twi 24,r3,5
    0x0F03000A,  // twi 24,r3,10
    0x0F030014,  // twi 24,r3,20
    0x0F04FFEC,  // twi 24,r4,-20
    0x0F04FFF6,  // twi 24,r4,-10
    0x0F04FFFB,  // twi 24,r4,-5
    0x0F040005,  // twi 24,r4,5
    0x0F040014,  // twi 24,r4,20
    0x0F23FFEC,  // twi 25,r3,-20
    0x0F23FFFB,  // twi 25,r3,-5
    0x0F230005,  // twi 25,r3,5
    0x0F23000A,  // twi 25,r3,10
    0x0F230014,  // twi 25,r3,20
    0x0F24FFEC,  // twi 25,r4,-20
    0x0F24FFF6,  // twi 25,r4,-10
    0x0F24FFFB,  // twi 25,r4,-5
    0x0F240005,  // twi 25,r4,5
    0x0F240014,  // twi 25,r4,20
    0x0F43FFEC,  // twi 26,r3,-20
    0x0F43FFFB,  // twi 26,r3,-5
    0x0F430005,  // twi 26,r3,5
    0x0F43000A,  // twi 26,r3,10
    0x0F430014,  // twi 26,r3,20
    0x0F44FFEC,  // twi 26,r4,-20
    0x0F44FFF6,  // twi 26,r4,-10
    0x0F44FFFB,  // twi 26,r4,-5
    0x0F440005,  // twi 26,r4,5
    0x0F440014,  // twi 26,r4,20
    0x0F63FFEC,  // twi 27,r3,-20
    0x0F63FFFB,  // twi 27,r3,-5
    0x0F630005,  // twi 27,r3,5
    0x0F63000A,  // twi 27,r3,10
    0x0F630014,  // twi 27,r3,20
    0x0F64FFEC,  // twi 27,r4,-20
    0x0F64FFF6,  // twi 27,r4,-10
    0x0F64FFFB,  // twi 27,r4,-5
    0x0F640005,  // twi 27,r4,5
    0x0F640014,  // twi 27,r4,20
    0x0F83FFEC,  // twi 28,r3,-20
    0x0F83FFF6,  // twi 28,r3,-10
    0x0F83FFFB,  // twi 28,r3,-5
    0x0F830005,  // twi 28,r3,5
    0x0F83000A,  // twi 28,r3,10
    0x0F830014,  // twi 28,r3,20
    0x0F84FFEC,  // twi 28,r4,-20
    0x0F84FFF6,  // twi 28,r4,-10
    0x0F84FFFB,  // twi 28,r4,-5
    0x0F840005,  // twi 28,r4,5
    0x0F84000A,  // twi 28,r4,10
    0x0F840014,  // twi 28,r4,20
    0x0FA3FFEC,  // twi 29,r3,-20
    0x0FA3FFF6,  // twi 29,r3,-10
    0x0FA3FFFB,  // twi 29,r3,-5
    0x0FA30005,  // twi 29,r3,5
    0x0FA3000A,  // twi 29,r3,10
    0x0FA30014,  // twi 29,r3,20
    0x0FA4FFEC,  // twi 29,r4,-20
    0x0FA4FFF6,  // twi 29,r4,-10
    0x0FA4FFFB,  // twi 29,r4,-5
    0x0FA40005,  // twi 29,r4,5
    0x0FA4000A,  // twi 29,r4,10
    0x0FA40014,  // twi 29,r4,20
    0x0FC3FFEC,  // twi 30,r3,-20
    0x0FC3FFF6,  // twi 30,r3,-10
    0x0FC3FFFB,  // twi 30,r3,-5
    0x0FC30005,  // twi 30,r3,5
    0x0FC3000A,  // twi 30,r3,10
    0x0FC30014,  // twi 30,r3,20
    0x0FC4FFEC,  // twi 30,r4,-20
    0x0FC4FFF6,  // twi 30,r4,-10
    0x0FC4FFFB,  // twi 30,r4,-5
    0x0FC40005,  // twi 30,r4,5
    0x0FC4000A,  // twi 30,r4,10
    0x0FC40014,  // twi 30,r4,20
    0x0FE3FFEC,  // twi 31,r3,-20
    0x0FE3FFF6,  // twi 31,r3,-10
    0x0FE3FFFB,  // twi 31,r3,-5
    0x0FE30005,  // twi 31,r3,5
    0x0FE3000A,  // twi 31,r3,10
    0x0FE30014,  // twi 31,r3,20
    0x0FE4FFEC,  // twi 31,r4,-20
    0x0FE4FFF6,  // twi 31,r4,-10
    0x0FE4FFFB,  // twi 31,r4,-5
    0x0FE40005,  // twi 31,r4,5
    0x0FE4000A,  // twi 31,r4,10
    0x0FE40014,  // twi 31,r4,20
};

static void trap_handler(PPCState *state)
{
    ASSERT(state);
    ASSERT(state->nia < 0x10000);
    ASSERT((state->nia & 3) == 0);
    ASSERT(trap_count < lenof(trap_insns));

    const uint32_t insn = bswap_be32(((uint32_t *)memory)[state->nia / 4]);
    trap_insns[trap_count++] = insn;

    state->nia += 4;  // Continue with the next instruction.
}


int main(void)
{
    if (!binrec_host_supported(binrec_native_arch())) {
        printf("Skipping test because native architecture not supported.\n");
        return EXIT_SUCCESS;
    }

    EXPECT(memory = malloc(0x10000));
    const uint32_t start_address = 0x1000;
    memcpy_be32(memory + start_address, ppc_code, sizeof(ppc_code));

    PPCState state;
    memset(&state, 0, sizeof(state));
    state.trap_handler = trap_handler;

    if (!call_guest_code(BINREC_ARCH_PPC_7XX, &state, memory, start_address,
                         0, 0, 0, 0, 0)) {
        const char *log_messages = get_log_messages();
        if (log_messages) {
            fputs(log_messages, stdout);
        }
        FAIL("Failed to execute guest code");
    }
    EXPECT_STREQ(get_log_messages(), NULL);

    EXPECT_EQ(state.gpr[3], (uint32_t)-10);
    EXPECT_EQ(state.gpr[4], 10);
    for (int i = 0; i < trap_count; i++) {
        bool found = false;
        for (int j = 0; j < lenof(expected_traps); j++) {
            if (trap_insns[i] == expected_traps[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            FAIL("Instruction %08X trapped but should not have", trap_insns[i]);
        }
    }
    for (int i = 0; i < lenof(expected_traps); i++) {
        bool found = false;
        for (int j = 0; j < trap_count; j++) {
            if (expected_traps[i] == trap_insns[j]) {
                found = true;
                break;
            }
        }
        if (!found) {
            FAIL("Instruction %08X did not trap but should have",
                 expected_traps[i]);
        }
    }
    ASSERT(trap_count == lenof(expected_traps));

    free(memory);
    return EXIT_SUCCESS;
}
