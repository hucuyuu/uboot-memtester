#!/bin/sh

set -eu

cd "$(dirname "$0")"

CROSS_COMPILE="${CROSS_COMPILE:-aarch64-linux-gnu-}"
CC="${CC:-${CROSS_COMPILE}gcc}"
LD="${LD:-${CROSS_COMPILE}ld}"
OBJCOPY="${OBJCOPY:-${CROSS_COMPILE}objcopy}"
OBJDUMP="${OBJDUMP:-${CROSS_COMPILE}objdump}"

OBJS=start.o
"$CC" -c -nostdlib -ffreestanding -o start.o start.S
for src in diagnostics.c main.c platform.c tests.c tests_basic.c tests_extended.c; do
    obj="${src%.c}.o"
    "$CC" -c -nostdlib -ffreestanding -fno-builtin -fno-stack-protector \
        -fno-pie -fno-pic -O2 -Wall -o "$obj" "$src"
    OBJS="$OBJS $obj"
done

"$LD" -T memtester_uboot.ld -o memtester_uboot.elf $OBJS
"$OBJCOPY" -O binary memtester_uboot.elf memtester_uboot.bin
"$OBJDUMP" -d memtester_uboot.elf | grep -A3 "<_entry>:" || true

echo "Built: memtester_uboot.bin"
