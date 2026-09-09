/* SPDX-License-Identifier: GPL-2.0-only */
/* Classic memtester comparison patterns and narrow-write tests. */

#include "memtester.h"

static union {
    unsigned char bytes[UL_LEN/8];
    ul val;
} mword8;

static union {
    unsigned short u16s[UL_LEN/16];
    ul val;
} mword16;

int compare_regions(ulv *bufa, ulv *bufb, unsigned long count)
{
    int r = 0;
    unsigned long i;
    ulv *p1 = bufa;
    ulv *p2 = bufb;

    for (i = 0; i < count; i++, p1++, p2++) {
        ul va = *p1;
        ul vb = *p2;
        if (va != vb) {
            diag_record((ul)p1, (ul)p2, va, vb);
            if (diag_can_print())
                uprintf("  FAILURE [%s]: A[%x]=%x != B[%x]=%x (offset %x) xor=%x\n",
                        diag_current_name(),
                        (ul)p1, va, (ul)p2, vb,
                        (ul)(i * sizeof(ul)), va ^ vb);
            r = -1;
        }
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return r;
}

int test_stuck_address(ulv *bufa, unsigned long count)
{
    ulv *p1;
    unsigned int j;
    unsigned long i;

    for (j = 0; j < 16; j++) {
        p1 = bufa;
        for (i = 0; i < count; i++) {
            *p1 = ((j + i) % 2) == 0 ? (ul)p1 : ~((ul)p1);
            *p1++;
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        p1 = bufa;
        for (i = 0; i < count; i++) {
            ul expect = (((j + i) % 2) == 0 ? (ul)p1 : ~((ul)p1));
            ul got = *p1;
            if (got != expect) {
                diag_record((ul)p1, 0, expect, got);
                if (diag_can_print())
                    uprintf("\n  FAILURE [%s]: bad address line at %x (offset %x) want=%x got=%x\n",
                            diag_current_name(), (ul)p1,
                            (ul)(i * sizeof(ul)), expect, got);
                return -1;
            }
            *p1++;
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
    }
    return 0;
}

int test_random_value(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = rand_ul();
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_xor_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ ^= q;
        *p2++ ^= q;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_sub_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ -= q;
        *p2++ -= q;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_mul_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ *= q;
        *p2++ *= q;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_div_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    if (!q) q++;

    for (i = 0; i < count; i++) {
        *p1++ /= q;
        *p2++ /= q;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_or_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ |= q;
        *p2++ |= q;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_and_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ &= q;
        *p2++ &= q;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_seqinc_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1 = bufa, *p2 = bufb;
    unsigned long i;
    ul q = rand_ul();

    for (i = 0; i < count; i++) {
        *p1++ = *p2++ = (i + q);
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return compare_regions(bufa, bufb, count);
}

int test_solidbits_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j;

    for (j = 0; j < 64; j++) {
        ul q = (j % 2) == 0 ? UL_ONEBITS : 0;
        p1 = bufa;
        p2 = bufb;
        for (i = 0; i < count; i++)
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        if (compare_regions(bufa, bufb, count))
            return -1;
        if ((j & 0xF) == 0xF)
            uprintf("  Solid %d/64\n", (unsigned long)(j + 1));
    }
    return 0;
}

int test_checkerboard_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j;

    for (j = 0; j < 64; j++) {
        ul q = (j % 2) == 0 ? CHECKERBOARD1 : CHECKERBOARD2;
        p1 = bufa;
        p2 = bufb;
        for (i = 0; i < count; i++)
            *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
        if (compare_regions(bufa, bufb, count))
            return -1;
        if ((j & 0xF) == 0xF)
            uprintf("  Checker %d/64\n", (unsigned long)(j + 1));
    }
    return 0;
}

int test_blockseq_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j;

    for (j = 0; j < 256; j++) {
        p1 = bufa;
        p2 = bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul)j;
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        if (compare_regions(bufa, bufb, count))
            return -1;
        if ((j & 0xF) == 0xF)
            uprintf("  Block Seq %d/256\n", (unsigned long)(j + 1));
    }
    return 0;
}

int test_walkbits0_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j;

    for (j = 0; j < UL_LEN; j++) {
        p1 = bufa;
        p2 = bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = ~((ul)1 << j);
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        if (compare_regions(bufa, bufb, count))
            return -1;
        if ((j & 0xF) == 0xF)
            uprintf("  Walk0 %d/64\n", (unsigned long)(j + 1));
    }
    return 0;
}

int test_walkbits1_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j;

    for (j = 0; j < UL_LEN; j++) {
        p1 = bufa;
        p2 = bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (ul)1 << j;
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        if (compare_regions(bufa, bufb, count))
            return -1;
        if ((j & 0xF) == 0xF)
            uprintf("  Walk1 %d/64\n", (unsigned long)(j + 1));
    }
    return 0;
}

int test_bitspread_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j;

    for (j = 0; j < UL_LEN; j++) {
        p1 = bufa;
        p2 = bufb;
        for (i = 0; i < count; i++) {
            *p1++ = *p2++ = (i % 2) == 0 ? ((ul)1 << j) : ((ul)1 << (j + 1));
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        if (compare_regions(bufa, bufb, count))
            return -1;
        if ((j & 0xF) == 0xF)
            uprintf("  Spread %d/64\n", (unsigned long)(j + 1));
    }
    return 0;
}

int test_bitflip_comparison(ulv *bufa, ulv *bufb, unsigned long count)
{
    ulv *p1, *p2;
    unsigned long i;
    unsigned int j, k;
    ul q;

    for (k = 0; k < 2; k++) {
        for (j = 0; j < UL_LEN; j++) {
            q = (ul)1 << j;
            p1 = bufa;
            p2 = bufb;
            for (i = 0; i < count; i++)
                *p1++ = *p2++ = (i % 2) == 0 ? q : ~q;
            if (compare_regions(bufa, bufb, count))
                return -1;
            if (k == 1) {
                p1 = bufa;
                p2 = bufb;
                for (i = 0; i < count; i++)
                    *p1++ = *p2++ = (i % 2) == 0 ? ~q : q;
                if (compare_regions(bufa, bufb, count))
                    return -1;
            }
            if ((j & 0xF) == 0xF)
                uprintf("  BitFlip %d/%d\n",
                        (unsigned long)(k * 64 + j + 1), (unsigned long)128);
        }
    }
    return 0;
}

int test_8bit_wide_random(ulv *bufa, ulv *bufb, unsigned long count)
{
    u8v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b;
    unsigned long i;

    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u8v *)bufa;
            p2 = bufb;
        } else {
            p1 = (u8v *)bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword8.bytes;
            *p2++ = mword8.val = rand_ul();
            for (b = 0; b < UL_LEN/8; b++)
                *p1++ = *t++;
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        if (compare_regions(bufa, bufb, count))
            return -1;
        uprintf("  8-bit Writes %d/2\n", (unsigned long)(attempt + 1));
    }
    return 0;
}

int test_16bit_wide_random(ulv *bufa, ulv *bufb, unsigned long count)
{
    u16v *p1, *t;
    ulv *p2;
    int attempt;
    unsigned int b;
    unsigned long i;

    for (attempt = 0; attempt < 2; attempt++) {
        if (attempt & 1) {
            p1 = (u16v *)bufa;
            p2 = bufb;
        } else {
            p1 = (u16v *)bufb;
            p2 = bufa;
        }
        for (i = 0; i < count; i++) {
            t = mword16.u16s;
            *p2++ = mword16.val = rand_ul();
            for (b = 0; b < UL_LEN/16; b++)
                *p1++ = *t++;
            if ((i & WDT_RESET_INTERVAL) == 0)
                watchdog_reset();
        }
        if (compare_regions(bufa, bufb, count))
            return -1;
        uprintf("  16-bit Writes %d/2\n", (unsigned long)(attempt + 1));
    }
    return 0;
}
