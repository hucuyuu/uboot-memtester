/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef MEMTESTER_UBOOT_H
#define MEMTESTER_UBOOT_H

#include <stddef.h>

#include "board.h"

typedef unsigned long       ul;
typedef unsigned long long  ull;
typedef unsigned long volatile ulv;
typedef unsigned char volatile u8v;
typedef unsigned short volatile u16v;

#define UL_LEN        64
#define UL_ONEBITS    0xffffffffffffffffUL
#define CHECKERBOARD1 0x5555555555555555UL
#define CHECKERBOARD2 0xaaaaaaaaaaaaaaaaUL

#define WDT_RESET_INTERVAL 0x100000UL
#define DIAG_PRINT_CAP     16

struct memtest {
    const char *name;
    int (*fp_single)(ulv *, unsigned long);
    int (*fp_dual)(ulv *, ulv *, unsigned long);
};

void uprintf(const char *fmt, ...);
void watchdog_stop(void);
void watchdog_reset(void);

void timer_init(void);
unsigned long timer_now(void);
unsigned long timer_frequency(void);
unsigned long timer_program_start(void);
unsigned long timer_ms_since(unsigned long t0);
void delay_ms(unsigned long ms);

unsigned long rand_ul(void);

void diag_set_region(unsigned long base, unsigned long size);
void diag_set_pass(unsigned long pass);
void diag_begin_test(int test_idx);
const char *diag_current_name(void);
void diag_record(unsigned long addr_a, unsigned long addr_b,
                 unsigned long want, unsigned long got);
int diag_can_print(void);
unsigned long diag_suppressed(void);
void diag_print(void);

int compare_regions(ulv *bufa, ulv *bufb, unsigned long count);
int test_stuck_address(ulv *bufa, unsigned long count);
int test_random_value(ulv *bufa, ulv *bufb, unsigned long count);
int test_xor_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_sub_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_mul_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_div_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_or_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_and_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_seqinc_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_solidbits_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_checkerboard_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_blockseq_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_walkbits0_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_walkbits1_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_bitspread_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_bitflip_comparison(ulv *bufa, ulv *bufb, unsigned long count);
int test_8bit_wide_random(ulv *bufa, ulv *bufb, unsigned long count);
int test_16bit_wide_random(ulv *bufa, ulv *bufb, unsigned long count);
int test_alias_ca(ulv *bufa, ulv *bufb, unsigned long count);
int test_march_c(ulv *bufa, ulv *bufb, unsigned long count);
int test_retention(ulv *bufa, ulv *bufb, unsigned long count);

const struct memtest *memtest_at(unsigned long idx);
const char *memtest_name(unsigned long idx);
unsigned long memtest_count(void);
const unsigned char *memtest_smoke_selection(unsigned long *count);

#endif
