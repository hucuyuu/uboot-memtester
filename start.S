/*
 * Assembly entry point for memtester U-Boot standalone
 * This MUST be the first code in the binary.
 * U-Boot's 'go' command jumps to the load address (0x100000).
 *
 * We use a separate .S file because GCC's naked attribute
 * is not reliably supported when targeting linux.
 */

    .section .text.start, "ax"
    .global _entry
    .type _entry, %function

_entry:
    /* Set up stack pointer at 8MB (well above code at 1MB) */
    mov  x0, #0x800000
    mov  sp, x0

    /* Zero .bss: U-Boot 'go' does not clear it for standalone apps.
     * Without this, static counters (diag_total, diag_ring, ...) hold
     * DRAM garbage and diag_print() crashes on a bogus ring entry. */
    ldr  x1, =__bss_start
    ldr  x2, =__bss_end
    mov  x3, #0
.Lbss_zero:
    cmp  x1, x2
    b.hs .Lbss_done
    sub  x4, x2, x1
    cmp  x4, #8
    b.lo .Lbss_byte
    str  x3, [x1], #8
    b    .Lbss_zero
.Lbss_byte:
    strb wzr, [x1], #1
    b    .Lbss_zero
.Lbss_done:

    /* Call C entry point */
    bl   _start

    /* Return to U-Boot */
    ret

    .size _entry, . - _entry
