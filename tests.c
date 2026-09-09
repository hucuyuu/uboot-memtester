/* SPDX-License-Identifier: GPL-2.0-only */
/* Ordered test registry and the subset used by SMOKE mode. */

#include "memtester.h"

static const struct memtest tests[] = {
    { "Stuck Address",        test_stuck_address,        NULL },
    { "Address Aliasing",     NULL,                      test_alias_ca },
    { "March C-",             NULL,                      test_march_c },
    { "Random Value",         NULL,                      test_random_value },
    { "Compare XOR",          NULL,                      test_xor_comparison },
    { "Compare SUB",          NULL,                      test_sub_comparison },
    { "Compare MUL",          NULL,                      test_mul_comparison },
    { "Compare DIV",          NULL,                      test_div_comparison },
    { "Compare OR",           NULL,                      test_or_comparison },
    { "Compare AND",          NULL,                      test_and_comparison },
    { "Sequential Increment", NULL,                      test_seqinc_comparison },
    { "Solid Bits",           NULL,                      test_solidbits_comparison },
    { "Block Sequential",     NULL,                      test_blockseq_comparison },
    { "Checkerboard",         NULL,                      test_checkerboard_comparison },
    { "Bit Spread",           NULL,                      test_bitspread_comparison },
    { "Bit Flip",             NULL,                      test_bitflip_comparison },
    { "Walking Ones",         NULL,                      test_walkbits1_comparison },
    { "Walking Zeroes",       NULL,                      test_walkbits0_comparison },
    { "8-bit Writes",         NULL,                      test_8bit_wide_random },
    { "16-bit Writes",        NULL,                      test_16bit_wide_random },
    { "Retention",            NULL,                      test_retention },
};

static const unsigned char smoke_selection[] = { 0, 1, 3, 11, 13, 16 };

const struct memtest *memtest_at(unsigned long idx)
{
    if (idx >= memtest_count())
        return NULL;
    return &tests[idx];
}

const char *memtest_name(unsigned long idx)
{
    const struct memtest *test = memtest_at(idx);
    return test ? test->name : "";
}

unsigned long memtest_count(void)
{
    return sizeof(tests) / sizeof(tests[0]);
}

const unsigned char *memtest_smoke_selection(unsigned long *count)
{
    if (count)
        *count = sizeof(smoke_selection) / sizeof(smoke_selection[0]);
    return smoke_selection;
}
