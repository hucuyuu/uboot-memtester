# U-Boot memtester

A standalone AArch64 memory tester for U-Boot, ported from memtester 4.5.1.
The upstream project is [memtester 4.5.1 by Charles
Cazabon](https://pyropus.ca/software/memtester/). The GPL-2.0-only license is
preserved in [LICENSE](LICENSE).

The current build targets the Broadcom BCM47722 platform and hardcodes the
PL011 UART and low-memory defaults used by the target board. Output goes to
the UART as configured by U-Boot; the tester does not re-initialize it. Edit
the source constants before use on different hardware.

## Build

```bash
./build.sh
```

Set `CROSS_COMPILE` to match your AArch64 toolchain, for example:

```bash
CROSS_COMPILE=aarch64-none-linux-gnu- ./build.sh
```

## Run

Load the generated `memtester_uboot.bin` at `0x100000`:

```text
tftpboot 0x100000 memtester_uboot.bin
go 0x100000
```

Without a configuration block, the program runs infinite SOAK passes over the
default low-memory region. The default test set is 21 patterns (18 classic
memtester tests plus Aliasing, March, and Retention). The tester never
returns to U-Boot:

- `SOAK` (default) runs infinite passes; stop it with a reset or power
  cycle.
- `SMOKE` and `FULL` run a single pass, print `System halted.`, and spin;
  power-cycle the board to reboot.

On startup the tester disables the hardware watchdog so long runs are not
reset by U-Boot inactivity. Each pass ends with a `RESULT:` line and a
`DIAG:` block (per-test, bit, lane, and segment failure counts plus a
rolling failure snapshot) for post-run analysis.

## Configuration block

Before `go`, the tester reads eight 32-bit words from `0x01000000`. If the
block is absent or invalid, it falls back to infinite `SOAK` mode.

| Word | Meaning |
| ---: | --- |
| 0-1 | Magic value `0x4d543132` |
| 2 | Mode: `0`=SOAK, `1`=SMOKE, `2`=FULL |
| 3-4 | Test start address (low/high 32 bits) |
| 5-6 | Test size in bytes (low/high 32 bits) |
| 7 | XOR of words 0 through 6 |

The start and size must both be at least 2 MB, and the region must end at or
below `0x7cbc8000`. The tester overwrites this memory while it runs.

This example selects a single-pass 64 MB SMOKE test:

```text
mw.l 0x01000000 0x4d543132
mw.l 0x01000004 0x4d543132
mw.l 0x01000008 0x00000001
mw.l 0x0100000c 0x01000000
mw.l 0x01000010 0x00000000
mw.l 0x01000014 0x04000000
mw.l 0x01000018 0x00000000
mw.l 0x0100001c 0x05000001
go 0x100000
```

## License

GPL-2.0-only. This is a U-Boot standalone port of memtester 4.5.1; see
[LICENSE](LICENSE) and the source headers.
