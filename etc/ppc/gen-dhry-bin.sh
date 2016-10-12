#!/bin/sh
#
# libbinrec: a recompiling translator for machine code
# Copyright (c) 2016 Andrew Church <achurch@achurch.org>
#
# This software may be copied and redistributed under certain conditions;
# see the file "COPYING" in the source code distribution for details.
# NO WARRANTY is provided with this software.
#

# This script builds the Dhrystone benchmark for 32-bit PowerPC and
# generates a blob source for building into the benchmark tool.
# Run from the top-level source directory as:
#     $0 {noopt|opt} >benchmarks/blobs/ppc32-dhry_{noopt|opt}.c
# If necessary, set environment variables as follows:
#     PPC_CC        32-bit PowerPC C cross-compiler
#     PPC_LD        "ld" from the binutils package, built for ppc32
#     PPC_NM        "nm" from the binutils package, built for ppc32
#     PPC_OBJCOPY   "objcopy" from the binutils package, built for ppc32

set -e

name=""
optflags=""
case "$1" in
    noopt)
        name="ppc32_dhry_noopt"
        ;;
    opt)
        name="ppc32_dhry_opt"
        optflags="-O2 -fno-inline"
        ;;
esac
if ! test -n "$name"; then
    echo >&2 "Usage: $0 {noopt|opt} >ppc32-dhry_{noopt|opt}.c"
    exit 1
fi

PPC_CC="${PPC_AS-powerpc-eabi-gcc}"
PPC_LD="${PPC_LD-powerpc-eabi-ld}"
PPC_NM="${PPC_NM-powerpc-eabi-nm}"
PPC_OBJCOPY="${PPC_OBJCOPY-powerpc-eabi-objcopy}"

tempdir="$(mktemp -d)"; test -n "$tempdir" -a -d "$tempdir"
trap "rm -r '$tempdir'" EXIT SIGHUP SIGINT SIGQUIT SIGTERM

base=0x100000
(
    set -x
    "$PPC_CC" -mcpu=750 -std=c89 -Ibenchmarks/library $optflags \
        -DBENCHMARK_ONLY \
        -o "$tempdir/dhry_1.o" -c benchmarks/dhrystone/dhry_1.c
    "$PPC_CC" -mcpu=750 -std=c89 -Ibenchmarks/library $optflags \
        -DBENCHMARK_ONLY \
        -o "$tempdir/dhry_2.o" -c benchmarks/dhrystone/dhry_2.c
    "$PPC_CC" -mcpu=750 -std=c99 -O3 -Wall -I. \
        -o "$tempdir/memcpy.o" -c benchmarks/library/memcpy.c
    "$PPC_CC" -mcpu=750 -std=c99 -O3 -Wall -I. \
        -o "$tempdir/strcmp.o" -c benchmarks/library/strcmp.c
    "$PPC_CC" -mcpu=750 -std=c99 -O3 -Wall -I. \
        -o "$tempdir/strcpy.o" -c benchmarks/library/strcpy.c
    "$PPC_LD" -Ttext=${base} --defsym _start=${base} \
        -o "$tempdir/dhry" "$tempdir/dhry_1.o" "$tempdir/dhry_2.o" \
        "$tempdir/memcpy.o" "$tempdir/strcmp.o" "$tempdir/strcpy.o"
    "$PPC_OBJCOPY" -O binary "$tempdir/dhry" "$tempdir/dhry.bin"
)

entry=$("$PPC_NM" "$tempdir/dhry" | grep ' T dhry_main$' | cut -c1-8)
entry=$((0x$entry - $base))

end=$("$PPC_NM" "$tempdir/dhry" | grep ' B __end$' | cut -c1-8)
reserve=$((0x$end))

echo "#include \"benchmarks/blobs.h\""
echo "static const uint8_t ${name}_bin[] = {";
perl <"$tempdir/dhry.bin" \
    -e 'while (read(STDIN, $_, 16)) {print join("", map {sprintf("%d,",$_)} unpack("C*", $_)) . "\n"}'
echo "};";
echo "const Blob ${name} = {"
echo "    .data = ${name}_bin,"
echo "    .size = sizeof(${name}_bin),"
echo "    .reserve = 0x$(printf %X ${reserve}),"
echo "    .base = ${base},"
echo "    .entry = 0x$(printf %X ${entry}),"
echo "};";
