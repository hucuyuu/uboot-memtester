/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Board-specific constants for the BCM47722 AArch64 standalone build.
 * Update these values before using the tester on different hardware.
 */

#ifndef MEMTESTER_BOARD_H
#define MEMTESTER_BOARD_H

/* PL011 debug UART. */
#define UART0_BASE   0xFF812000UL

/* Hardware watchdog used to prevent U-Boot inactivity resets. */
#define WDT_BASE     0xFF800480UL

/* Startup parameter block, written from U-Boot before `go`. */
#define PARAM_ADDR   0x01000000UL

/* Safe low-memory test window on the current 2 GB board. */
#define TEST_START   0x01000000UL
#define TEST_SIZE    0x7BBC8000UL
#define TEST_END     0x7CBC8000UL
#define SMOKE_SIZE   0x04000000UL

#endif
