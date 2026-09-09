/* SPDX-License-Identifier: GPL-2.0-only */
/* Address-aliasing, march, and retention tests added in the v1.22 line. */

#include "memtester.h"

/* Verify that address P and P XOR 2^k are independent cells. Sweep from
 * k=3 because k=0..2 produce unaligned 64-bit stores that straddle P's own
 * word and self-corrupt it. Byte lanes are covered by narrow-write tests. */
int test_alias_ca(ulv *bufa, ulv *bufb, unsigned long count)
{
    unsigned long region_bytes = count * 2UL * sizeof(ul);
    unsigned long k, bits_checked = 0;
    unsigned long f;
    ul X = 0xA5A5A5A5A5A5A5A5UL;
    ul Y = ~X;
    int rc = 0;

    (void)bufb;

    for (k = 3; (1UL << k) < region_bytes; k++) {
        unsigned long stride = 1UL << k;

        for (f = 1; f <= 7; f += 2) {
            unsigned long off = (region_bytes / 8UL) * f & ~63UL;
            ulv *P, *Q;

            if (off + sizeof(ul) > region_bytes)
                continue;
            P = (ulv *)((ul)bufa + off);
            Q = (ulv *)((ul)P ^ stride);
            if ((ul)Q & (sizeof(ul) - 1UL))
                continue;
            if ((ul)Q < (ul)bufa ||
                (ul)Q > (ul)bufa + region_bytes - sizeof(ul) || Q == P)
                continue;

            *P = X;
            *Q = Y;
            if (*P != X) {
                diag_record((ul)P, (ul)Q, X, *P);
                if (diag_can_print())
                    uprintf("  FAILURE [%s]: bit%d write Q=%x corrupted P: want=%x got=%x\n",
                            diag_current_name(), (unsigned long)k,
                            (ul)Q, X, *P);
                rc = -1;
            }
            if (*Q != Y) {
                diag_record((ul)Q, (ul)P, Y, *Q);
                if (diag_can_print())
                    uprintf("  FAILURE [%s]: bit%d Q=%x readback: want=%x got=%x\n",
                            diag_current_name(), (unsigned long)k,
                            (ul)Q, Y, *Q);
                rc = -1;
            }

            *P = Y;
            *Q = X;
            if (*P != Y) {
                diag_record((ul)P, (ul)Q, Y, *P);
                if (diag_can_print())
                    uprintf("  FAILURE [%s]: bit%d swapped write Q corrupted P: want=%x got=%x\n",
                            diag_current_name(), (unsigned long)k,
                            (ul)Q, Y, *P);
                rc = -1;
            }
            if (*Q != X) {
                diag_record((ul)Q, (ul)P, X, *Q);
                if (diag_can_print())
                    uprintf("  FAILURE [%s]: bit%d swapped Q=%x readback: want=%x got=%x\n",
                            diag_current_name(), (unsigned long)k,
                            (ul)Q, X, *Q);
                rc = -1;
            }

            watchdog_reset();
            bits_checked++;
        }
    }

    if (bits_checked == 0) {
        uprintf("  (region too small for CA aliasing sweep)\n");
        return 0;
    }
    uprintf("  CA aliasing: %d (bit,base) cells checked\n", bits_checked);
    return rc;
}

static void march_fill(ulv *base, unsigned long n, ul pat)
{
    unsigned long i;
    for (i = 0; i < n; i++) {
        base[i] = pat;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
}

static int march_rw(ulv *base, unsigned long n, ul expect, ul newpat,
                    int descending)
{
    unsigned long i;
    int rc = 0;

    for (i = 0; i < n; i++) {
        unsigned long k = descending ? (n - 1 - i) : i;
        ulv *p = base + k;
        ul got = *p;

        if (got != expect) {
            diag_record((ul)p, 0, expect, got);
            if (diag_can_print())
                uprintf("  FAILURE [%s]: @%x want=%x got=%x\n",
                        diag_current_name(), (ul)p, expect, got);
            rc = -1;
        }
        *p = newpat;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return rc;
}

static int march_rd(ulv *base, unsigned long n, ul expect, int descending)
{
    unsigned long i;
    int rc = 0;

    for (i = 0; i < n; i++) {
        unsigned long k = descending ? (n - 1 - i) : i;
        ulv *p = base + k;
        ul got = *p;

        if (got != expect) {
            diag_record((ul)p, 0, expect, got);
            if (diag_can_print())
                uprintf("  FAILURE [%s]: @%x want=%x got=%x\n",
                        diag_current_name(), (ul)p, expect, got);
            rc = -1;
        }
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return rc;
}

int test_march_c(ulv *bufa, ulv *bufb, unsigned long count)
{
    unsigned long n = count * 2UL;
    int rc = 0;

    (void)bufb;

    uprintf("  March 1/6 (w A)\n");
    march_fill(bufa, n, 0UL);

    uprintf("  March 2/6 (rA,wB up)\n");
    if (march_rw(bufa, n, 0UL, UL_ONEBITS, 0)) rc = -1;

    uprintf("  March 3/6 (rB,wA up)\n");
    if (march_rw(bufa, n, UL_ONEBITS, 0UL, 0)) rc = -1;

    uprintf("  March 4/6 (rA,wB down)\n");
    if (march_rw(bufa, n, 0UL, UL_ONEBITS, 1)) rc = -1;

    uprintf("  March 5/6 (rB,wA down)\n");
    if (march_rw(bufa, n, UL_ONEBITS, 0UL, 1)) rc = -1;

    uprintf("  March 6/6 (rA down)\n");
    if (march_rd(bufa, n, 0UL, 1)) rc = -1;

    return rc;
}

int test_retention(ulv *bufa, ulv *bufb, unsigned long count)
{
    unsigned long n = count * 2UL;
    unsigned long i;
    int rc = 0;

    (void)bufb;

    for (i = 0; i < n; i++) {
        bufa[i] = (i & 1) ? CHECKERBOARD2 : CHECKERBOARD1;
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }

    uprintf("  Retention: region written, holding 5 s ...\n");
    delay_ms(5000);
    uprintf("  Retention: verifying ...\n");

    for (i = 0; i < n; i++) {
        ul expect = (i & 1) ? CHECKERBOARD2 : CHECKERBOARD1;
        ul got = bufa[i];
        if (got != expect) {
            diag_record((ul)(bufa + i), 0, expect, got);
            if (diag_can_print())
                uprintf("  FAILURE [%s]: @%x want=%x got=%x\n",
                        diag_current_name(), (ul)(bufa + i), expect, got);
            rc = -1;
        }
        if ((i & WDT_RESET_INTERVAL) == 0)
            watchdog_reset();
    }
    return rc;
}
