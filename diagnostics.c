/* SPDX-License-Identifier: GPL-2.0-only */
/* Failure histograms and the rolling failure snapshot used by all tests. */

#include "memtester.h"

#define DIAG_MAX_SNAP 32
#define DIAG_NSEG     8

struct fail_rec {
    unsigned long addr_a;
    unsigned long addr_b;
    unsigned long want;
    unsigned long got;
    unsigned long pass_no;
    unsigned char test_idx;
};

static unsigned long diag_bit_hist[64];
static unsigned long diag_lane[2];
static unsigned long diag_seg[DIAG_NSEG];
static unsigned long diag_per_test[32];
static unsigned long diag_total;
static unsigned long diag_fail_this_test;
static unsigned long diag_fail_printed;
static struct fail_rec diag_ring[DIAG_MAX_SNAP];
static unsigned long diag_ring_wr;
static unsigned long diag_ring_total;

static unsigned long g_region_base;
static unsigned long g_region_size;
static unsigned long g_pass_no;
static int g_cur_test_idx = -1;
static const char *g_cur_test_name = "";

void diag_set_region(unsigned long base, unsigned long size)
{
    g_region_base = base;
    g_region_size = size;
}

void diag_set_pass(unsigned long pass)
{
    g_pass_no = pass;
}

void diag_begin_test(int test_idx)
{
    g_cur_test_idx = test_idx;
    g_cur_test_name = test_idx >= 0 ? memtest_name((unsigned long)test_idx) : "";
    diag_fail_this_test = 0;
    diag_fail_printed = 0;
}

const char *diag_current_name(void)
{
    return g_cur_test_name;
}

unsigned long diag_suppressed(void)
{
    if (diag_fail_this_test > diag_fail_printed)
        return diag_fail_this_test - diag_fail_printed;
    return 0;
}

void diag_record(unsigned long addr_a, unsigned long addr_b,
                 unsigned long want, unsigned long got)
{
    unsigned long x = want ^ got;
    unsigned long segsize;
    struct fail_rec *r;
    int i;

    diag_total++;
    diag_fail_this_test++;
    if (g_cur_test_idx >= 0 && g_cur_test_idx < (int)(sizeof(diag_per_test) /
                                                     sizeof(diag_per_test[0])))
        diag_per_test[g_cur_test_idx]++;

    for (i = 0; i < 64; i++) {
        if (x & (1UL << i)) {
            diag_bit_hist[i]++;
            diag_lane[(i >> 3) & 1]++;
        }
    }

    if (g_region_size) {
        segsize = g_region_size / DIAG_NSEG;
        if (segsize && addr_a >= g_region_base &&
            addr_a < g_region_base + g_region_size)
            diag_seg[(addr_a - g_region_base) / segsize]++;
        if (segsize && addr_b >= g_region_base &&
            addr_b < g_region_base + g_region_size)
            diag_seg[(addr_b - g_region_base) / segsize]++;
    }

    r = &diag_ring[diag_ring_wr % DIAG_MAX_SNAP];
    r->addr_a = addr_a;
    r->addr_b = addr_b;
    r->want = want;
    r->got = got;
    r->pass_no = g_pass_no;
    r->test_idx = (unsigned char)(g_cur_test_idx < 0 ? 0 : g_cur_test_idx);
    diag_ring_wr++;
    diag_ring_total++;
}

int diag_can_print(void)
{
    if (diag_fail_printed < DIAG_PRINT_CAP) {
        diag_fail_printed++;
        return 1;
    }
    return 0;
}

void diag_print(void)
{
    int i;
    unsigned long n, s, idx, ntests = memtest_count();

    if (diag_total == 0) {
        uprintf("DIAG: failures=0 (bit/lane/segment counters all zero)\n");
        return;
    }

    uprintf("DIAG: total failures = %d\n", diag_total);
    uprintf("DIAG: lane0 (DQ[7:0])  errors = %d\n", diag_lane[0]);
    uprintf("DIAG: lane1 (DQ[15:8]) errors = %d\n", diag_lane[1]);

    uprintf("DIAG: bits:");
    for (i = 0; i < 64; i++)
        if (diag_bit_hist[i])
            uprintf(" b%d=%d", (unsigned long)i, diag_bit_hist[i]);
    uprintf("\n");

    uprintf("DIAG: segments (region/8):");
    for (i = 0; i < DIAG_NSEG; i++)
        uprintf(" %d", diag_seg[i]);
    uprintf("\n");

    uprintf("DIAG: per-test:");
    for (i = 0; i < (int)ntests; i++)
        if (diag_per_test[i])
            uprintf(" [%s]=%d", memtest_name((unsigned long)i),
                    diag_per_test[i]);
    uprintf("\n");

    n = diag_ring_total < 8 ? diag_ring_total : 8;
    uprintf("DIAG: last %d failure(s) of %d:\n", n, diag_ring_total);
    for (s = 0; s < n; s++) {
        struct fail_rec *r;
        idx = (diag_ring_wr - 1 - s) % DIAG_MAX_SNAP;
        r = &diag_ring[idx];
        uprintf("  pass%d [%s] A=%x B=%x want=%x got=%x\n",
                r->pass_no, memtest_name(r->test_idx),
                r->addr_a, r->addr_b, r->want, r->got);
    }
}
