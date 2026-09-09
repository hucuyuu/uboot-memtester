/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Minimal UART, timer, watchdog, and RNG services for the standalone app.
 */

#include "memtester.h"

#define UART_DR      (*(volatile unsigned int *)(UART0_BASE + 0x000))
#define UART_FR      (*(volatile unsigned int *)(UART0_BASE + 0x018))
#define UART_FR_TXFF (1u << 5)

static void uart_putc(char c)
{
    while (UART_FR & UART_FR_TXFF)
        ;
    UART_DR = (unsigned int)c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

static void uart_put_hex(unsigned long val)
{
    char buf[17];
    int i;
    for (i = 15; i >= 0; i--) {
        unsigned int nib = (unsigned int)((val >> (i * 4)) & 0xf);
        buf[15 - i] = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);
    }
    buf[16] = '\0';
    uart_puts(buf);
}

static void uart_put_dec(unsigned long val)
{
    char buf[21];
    int i = 20;
    buf[i--] = '\0';
    if (val == 0) {
        buf[i--] = '0';
    }
    while (val > 0) {
        buf[i--] = (char)('0' + (val % 10));
        val /= 10;
    }
    uart_puts(&buf[i + 1]);
}

/* Integer arguments are always passed as unsigned long. On AArch64, narrower
 * variadic integers cannot be relied on to have their upper bits defined. */
void uprintf(const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
            case 's': {
                const char *s = __builtin_va_arg(args, const char *);
                uart_puts(s ? s : "(null)");
                break;
            }
            case 'x': {
                unsigned long v = __builtin_va_arg(args, unsigned long);
                uart_puts("0x");
                uart_put_hex(v);
                break;
            }
            case 'd':
            case 'u':
            case 'l': {
                if (*fmt == 'l' && (fmt[1] == 'u' || fmt[1] == 'd'))
                    fmt++;
                if (*fmt == 'c') {
                    int c = __builtin_va_arg(args, int);
                    uart_putc((char)c);
                } else {
                    unsigned long v = __builtin_va_arg(args, unsigned long);
                    uart_put_dec(v);
                }
                break;
            }
            case 'c': {
                int c = __builtin_va_arg(args, int);
                uart_putc((char)c);
                break;
            }
            case '%':
                uart_putc('%');
                break;
            default:
                uart_putc('%');
                uart_putc(*fmt);
                break;
            }
        } else {
            if (*fmt == '\n')
                uart_putc('\r');
            uart_putc(*fmt);
        }
        fmt++;
    }

    __builtin_va_end(args);
}

static unsigned long g_timer_freq;
static unsigned long g_t_start;

unsigned long timer_now(void)
{
    unsigned long v;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(v));
    return v;
}

unsigned long timer_frequency(void)
{
    unsigned long v;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
}

void timer_init(void)
{
    g_timer_freq = timer_frequency();
    g_t_start = timer_now();
}

unsigned long timer_program_start(void)
{
    return g_t_start;
}

unsigned long timer_ms_since(unsigned long t0)
{
    unsigned long now = timer_now();
    if (!g_timer_freq)
        return 0;
    return ((now - t0) * 1000UL) / g_timer_freq;
}

void delay_ms(unsigned long ms)
{
    unsigned long t0, ticks;
    if (!g_timer_freq || ms == 0)
        return;
    t0 = timer_now();
    ticks = (g_timer_freq / 1000UL) * ms;
    while ((timer_now() - t0) < ticks)
        ;
}

#define WDT_VAL_REG   (*(volatile unsigned int *)(WDT_BASE + 0x00))
#define WDT_CTL_REG   (*(volatile unsigned int *)(WDT_BASE + 0x04))

void watchdog_stop(void)
{
    WDT_CTL_REG = 0x0000ee00;
    WDT_CTL_REG = 0x000000ee;
}

void watchdog_reset(void)
{
    /* Startup stop is proven to work; kicking can re-enable the WDT. */
}

static unsigned long rng_state = 0x123456789abcdef0UL;

unsigned long rand_ul(void)
{
    unsigned long x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DUL;
}
