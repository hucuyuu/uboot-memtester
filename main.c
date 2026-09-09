/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * U-Boot standalone entry point, runtime configuration, and test-loop
 * control for the BCM47722 AArch64 port.
 */

#include "memtester.h"

#define PARAM_MAGIC   0x4D543132UL
#define MODE_SOAK     0
#define MODE_SMOKE    1
#define MODE_FULL     2

/* Eight 32-bit words written at PARAM_ADDR before 'go'. Words 0 and 1 are
 * magic, 2 selects the mode, 3..6 are the 64-bit start/size, and 7 is the
 * XOR checksum. An invalid block silently selects SOAK and the defaults. */
static int parse_params(unsigned long *p_start, unsigned long *p_size,
                        int *p_mode)
{
    volatile unsigned int *pp = (volatile unsigned int *)PARAM_ADDR;
    unsigned int w[8];
    unsigned int csum;
    unsigned long start, size;
    int i;

    for (i = 0; i < 8; i++)
        w[i] = pp[i];

    if (w[0] != PARAM_MAGIC || w[1] != PARAM_MAGIC || w[2] > MODE_FULL)
        return 0;

    csum = w[0] ^ w[1] ^ w[2] ^ w[3] ^ w[4] ^ w[5] ^ w[6];
    if (csum != w[7])
        return 0;

    start = ((unsigned long)w[4] << 32) | (unsigned long)w[3];
    size  = ((unsigned long)w[6] << 32) | (unsigned long)w[5];

    if (start < 0x200000UL || size < 0x200000UL ||
        start > TEST_END || size > TEST_END - start)
        return 0;

    *p_start = start & ~7UL;
    *p_size  = size & ~15UL;
    *p_mode  = (int)w[2];
    return 1;
}

/*
 * Disabled reference MMU setup. If enabled, it builds AArch64 page tables and
 * turns on the MMU: 0x00000000-0x7fffffff and 0x100000000-0x17fffffff map as
 * normal cacheable memory, while 0xc0000000-0xffffffff maps as device memory.
 * It also programs MAIR/TCR/TTBR, invalidates the TLB, and enables the MMU at
 * EL1 or EL2. The tester currently runs with the mapping established by
 * U-Boot, so this path must not be re-enabled without validating the EL and
 * existing page-table state on the target board.
 */
#if 0
#define MMU_ATTR_DEV  0UL
#define MMU_ATTR_MEM  1UL
#define MMU_BLK_DEV   (1UL | (MMU_ATTR_DEV << 2) | (1UL << 10) | (3UL << 8) | \
                       (1UL << 53) | (1UL << 54))
#define MMU_BLK_MEM   (1UL | (MMU_ATTR_MEM << 2) | (1UL << 10) | (3UL << 8))
#define MMU_TBL(pa)   ((pa) | 3UL)
#define MMU_MAIR      (0x00UL | (0xFFUL << 8))
#define MMU_TCR       (16UL | (1UL << 8) | (1UL << 10) | (3UL << 12) | (2UL << 32))

static ul mmu_l0[512] __attribute__((aligned(4096)));
static ul mmu_l1[512] __attribute__((aligned(4096)));
static ul mmu_l2_low[1024] __attribute__((aligned(4096)));
static ul mmu_l2_dev[512] __attribute__((aligned(4096)));
static ul mmu_l2_high[1024] __attribute__((aligned(4096)));

static void mmu_clean_table(ul *table, unsigned long n)
{
    unsigned long i;
    for (i = 0; i < n; i += 8)
        __asm__ volatile("dc civac, %0" :: "r"(&table[i]));
    __asm__ volatile("dsb ish");
}

__attribute__((unused))
static void mmu_setup_self(void)
{
    unsigned long i, el;

    for (i = 0; i < 512; i++) {
        mmu_l0[i] = 0;
        mmu_l1[i] = 0;
        mmu_l2_dev[i] = 0;
    }
    for (i = 0; i < 1024; i++) {
        mmu_l2_low[i] = 0;
        mmu_l2_high[i] = 0;
    }

    mmu_l0[0] = MMU_TBL((ul)mmu_l1);
    mmu_l1[0] = MMU_TBL((ul)mmu_l2_low);
    mmu_l1[1] = MMU_TBL((ul)(mmu_l2_low + 512));
    mmu_l1[3] = MMU_TBL((ul)mmu_l2_dev);
    mmu_l1[4] = MMU_TBL((ul)mmu_l2_high);
    mmu_l1[5] = MMU_TBL((ul)(mmu_l2_high + 512));

    for (i = 0; i < 1024; i++)
        mmu_l2_low[i] = ((ul)i << 21) | MMU_BLK_MEM;
    for (i = 504; i < 512; i++)
        mmu_l2_dev[i] = (0xC0000000UL + ((ul)i << 21)) | MMU_BLK_DEV;
    for (i = 0; i < 1024; i++)
        mmu_l2_high[i] = (0x100000000UL + ((ul)i << 21)) | MMU_BLK_MEM;

    mmu_clean_table(mmu_l0, 512);
    mmu_clean_table(mmu_l1, 512);
    mmu_clean_table(mmu_l2_low, 1024);
    mmu_clean_table(mmu_l2_dev, 512);
    mmu_clean_table(mmu_l2_high, 1024);

    __asm__ volatile("mrs %0, CurrentEL" : "=r"(el));
    el = (el >> 2) & 3UL;
    if (el == 1) {
        __asm__ volatile(
            "ic iallu\n"
            "dsb ish\n"
            "msr ttbr0_el1, %0\n"
            "msr tcr_el1, %1\n"
            "msr mair_el1, %2\n"
            "isb\n"
            "tlbi alle1\n"
            "dsb ish\n"
            "isb\n"
            :: "r"((ul)mmu_l0), "r"(MMU_TCR), "r"(MMU_MAIR) : "memory");
    } else if (el == 2) {
        __asm__ volatile(
            "ic iallu\n"
            "dsb ish\n"
            "msr ttbr0_el2, %0\n"
            "msr tcr_el2, %1\n"
            "msr mair_el2, %2\n"
            "isb\n"
            "tlbi alle2\n"
            "dsb ish\n"
            "isb\n"
            :: "r"((ul)mmu_l0), "r"(MMU_TCR), "r"(MMU_MAIR) : "memory");
    } else {
        uprintf("ERROR: unsupported EL%ld for MMU setup\n", el);
    }
}
#endif

static int run_region(const char *label, ulv *bufa, ulv *bufb,
                      unsigned long count, unsigned long halflen,
                      const unsigned char *sel, unsigned long nsel)
{
    unsigned long s;
    int exit_code = 0;
    int n_ok = 0, n_fail = 0;

    uprintf("\n===== Region [%s] =====\n", label);
    uprintf("Buffer A        : %x (%d MB)\n", (ul)bufa, halflen >> 20);
    uprintf("Buffer B        : %x (%d MB)\n", (ul)bufb, halflen >> 20);
    uprintf("Word count      : %d\n", count);
    uprintf("Tests           : %d\n", nsel);
    uprintf("\n");

    uprintf("Memory probe (multiple points):\n");
    {
        struct { const char *name; ulv *addr; } probes[] = {
            { "bufA[0%]",  bufa },
            { "bufA[25%]", (ulv *)((ul)bufa + halflen / 4) },
            { "bufA[50%]", (ulv *)((ul)bufa + halflen / 2) },
            { "bufA[75%]", (ulv *)((ul)bufa + halflen * 3 / 4) },
            { "bufA[end]", (ulv *)((ul)bufa + halflen - sizeof(ul)) },
            { "bufB[0%]",  bufb },
            { "bufB[25%]", (ulv *)((ul)bufb + halflen / 4) },
            { "bufB[50%]", (ulv *)((ul)bufb + halflen / 2) },
            { "bufB[75%]", (ulv *)((ul)bufb + halflen * 3 / 4) },
            { "bufB[end]", (ulv *)((ul)bufb + halflen - sizeof(ul)) },
        };
        int np = sizeof(probes) / sizeof(probes[0]);
        int pi;
        int abort = 0;

        for (pi = 0; pi < np; pi++) {
            ul orig = *probes[pi].addr;
            *probes[pi].addr = 0xDEADBEEF12345678UL;
            if (*probes[pi].addr != 0xDEADBEEF12345678UL) {
                uprintf("  FAIL: %s @ %x\n",
                        probes[pi].name, (ul)probes[pi].addr);
                abort = 1;
            } else {
                *probes[pi].addr = orig;
                uprintf("  %s @ %x : OK\n",
                        probes[pi].name, (ul)probes[pi].addr);
            }
        }
        if (abort) {
            uprintf("\n*** ABORT: Memory probe failed! Reduce test size. ***\n");
            uprintf("RESULT: ABORT (%s probe failed)\n", label);
            uprintf("System halted. Power cycle to reboot.\n");
            while (1) { __asm__ volatile("wfi"); }
            return 0x08;
        }
    }
    uprintf("\n");

    for (s = 0; s < nsel; s++) {
        unsigned long i = sel ? (unsigned long)sel[s] : s;
        const struct memtest *test = memtest_at(i);
        unsigned long t0, ms;
        int result;

        if (!test)
            continue;

        uprintf("  %s: ", test->name);
        diag_begin_test((int)i);

        t0 = timer_now();
        if (test->fp_single)
            result = test->fp_single(bufa, count);
        else
            result = test->fp_dual(bufa, bufb, count);
        ms = timer_ms_since(t0);

        if (result != 0) {
            uprintf("FAILURE");
            exit_code |= 0x04;
            n_fail++;
        } else {
            uprintf("ok");
            n_ok++;
        }
        uprintf(" (%d.%d s)\n", ms / 1000UL, (ms % 1000UL) / 100UL);

        if (diag_suppressed())
            uprintf("  [%s]: +%d more failure(s) suppressed (print cap %d)\n",
                    test->name, diag_suppressed(),
                    (unsigned long)DIAG_PRINT_CAP);
    }

    uprintf("\n");
    if (exit_code == 0) {
        uprintf("=============================================\n");
        uprintf(" ALL TESTS PASSED [%s]\n", label);
        uprintf("=============================================\n");
    } else {
        uprintf("=============================================\n");
        uprintf(" TEST FAILURES DETECTED [%s] (code=%x)\n", label,
                (unsigned long)exit_code);
        uprintf("=============================================\n");
    }
    uprintf("RESULT: %s (%d/%d ok, %d failed) [%s]\n",
            exit_code == 0 ? "PASS" : "FAIL",
            (unsigned long)n_ok, nsel, (unsigned long)n_fail, label);
    diag_print();
    uprintf("\n");
    return exit_code;
}

unsigned long _start(void)
{
    unsigned long test_start = TEST_START;
    unsigned long test_size  = TEST_SIZE;
    unsigned long program_start;
    unsigned long bufsize, halflen, count;
    unsigned long nsel;
    const unsigned char *sel = NULL;
    ulv *bufa, *bufb;
    const char *mode_name = "SOAK";
    int mode = MODE_SOAK;
    int have_params;
    int rc;

    watchdog_stop();
    timer_init();
    program_start = timer_program_start();
    have_params = parse_params(&test_start, &test_size, &mode);

    test_start = (test_start + 7) & ~7UL;
    test_size  = test_size & ~15UL;

    if (mode == MODE_SMOKE) {
        mode_name = "SMOKE";
        sel = memtest_smoke_selection(&nsel);
        if (test_size > SMOKE_SIZE)
            test_size = SMOKE_SIZE;
    } else if (mode == MODE_FULL) {
        mode_name = "FULL";
        nsel = memtest_count();
    } else {
        nsel = memtest_count();
    }

    bufsize = test_size;
    halflen = bufsize / 2;
    count = halflen / sizeof(ul);
    bufa = (ulv *)test_start;
    bufb = (ulv *)(test_start + halflen);
    diag_set_region(test_start, bufsize);

    uprintf("\n");
    uprintf("=============================================\n");
    uprintf(" memtester U-Boot standalone v1.23\n");
    uprintf(" (ported from memtester 4.5.1, 64-bit)\n");
    uprintf(" (single-region LOW 0-2GB, 2GB board, SOAK+DIAG, alias k>=3 fix)\n");
    uprintf("=============================================\n");
    uprintf("\n");
    if (have_params)
        uprintf("PARAMS: config block @0x1000000 valid -> mode=%d\n",
                (unsigned long)mode);
    else
        uprintf("PARAMS: no valid config block -> defaults (SOAK)\n");
    uprintf("Mode        : %s (%s)\n", mode_name,
            mode == MODE_SOAK ? "infinite passes" : "single pass");
    uprintf("LOW region  : %x - %x (%d MB)\n", test_start,
            test_start + test_size, test_size >> 20);
    uprintf("Tests       : %d (%d classic + Aliasing/March/Retention)\n",
            nsel, (unsigned long)(mode == MODE_SMOKE ? 0 : 18));
    uprintf("Timer       : %d Hz\n", timer_frequency());
    uprintf("\n");

    {
        unsigned long pass = 1;

        while (1) {
            unsigned long pass_start = timer_now();
            unsigned long ms;

            if (mode == MODE_SOAK) {
                uprintf("\n=============================================\n");
                uprintf("   SOAK PASS %d\n", pass);
                uprintf("=============================================\n");
            }

            diag_set_pass(pass);
            rc = run_region(mode == MODE_SMOKE ? "SMOKE" : "LOW",
                            bufa, bufb, count, halflen, sel, nsel);
            ms = timer_ms_since(pass_start);

            if (mode == MODE_SOAK) {
                unsigned long total_ms = timer_ms_since(program_start);
                uprintf("\n[SOAK] PASS %d -> %s (pass %d.%d min, total %d.%d h)\n",
                        pass, rc == 0 ? "PASS" : "FAIL",
                        ms / 60000UL, (ms % 60000UL) / 6000UL,
                        total_ms / 3600000UL, (total_ms % 3600000UL) / 360000UL);
                uprintf("\n");
                pass++;
                continue;
            }

            {
                unsigned long total_ms = timer_ms_since(program_start);
                uprintf("\n[DONE] mode=%s total %d.%d min rc=%d\n",
                        mode_name, total_ms / 60000UL,
                        (total_ms % 60000UL) / 6000UL, (unsigned long)rc);
            }
            uprintf("System halted. Power cycle to reboot.\n");
            while (1) { __asm__ volatile("wfi"); }
        }
    }
}
