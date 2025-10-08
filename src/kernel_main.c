#include <stdint.h>
#include "rprintf.h"
#include "interrupt.h"
#include "keyboard.h"

#define VGA_W      80
#define VGA_H      25
#define VGA_COLOR  0x07
static volatile uint16_t *const VGA = (uint16_t*)0xB8000;
static int cur_row = 0, cur_col = 0;

static inline void vga_put_at(char ch, int r, int c) {
    VGA[r * VGA_W + c] = ((uint16_t)VGA_COLOR << 8) | (uint8_t)ch;
}

static void scroll_if_needed(void) {
    if (cur_row < VGA_H) return;
    for (int r = 1; r < VGA_H; ++r)
        for (int c = 0; c < VGA_W; ++c)
            VGA[(r - 1) * VGA_W + c] = VGA[r * VGA_W + c];
    for (int c = 0; c < VGA_W; ++c)
        VGA[(VGA_H - 1) * VGA_W + c] = ((uint16_t)VGA_COLOR << 8) | ' ';
    cur_row = VGA_H - 1;
}

int putc(int data) {
    char ch = (char)data;

    if (ch == '\n') {
        cur_row++; cur_col = 0;
    } else if (ch == '\r') {
        cur_col = 0;
    } else if (ch == '\b') {
        // move back and clear the cell
        if (cur_col > 0) {
            cur_col--;
            vga_put_at(' ', cur_row, cur_col);
        } else if (cur_row > 0) {
            cur_row--; cur_col = VGA_W - 1;
            vga_put_at(' ', cur_row, cur_col);
        }
    } else {
        // print any visible char, including space, and advance
        vga_put_at(ch, cur_row, cur_col);
        cur_col++;
        if (cur_col >= VGA_W) { cur_col = 0; cur_row++; }
    }

    scroll_if_needed();
    return 0;
}

static void delay(int ms) { for (volatile int i = 0; i < ms * 100000; i++) {} }

void kernel_main() {
    // clear screen
    for (int r = 0; r < VGA_H; ++r)
        for (int c = 0; c < VGA_W; ++c)
            VGA[r * VGA_W + c] = ((uint16_t)VGA_COLOR << 8) | ' ';

    esp_printf(putc, "===========================================\r\n");
    esp_printf(putc, "  Custom OS - COMP 310 Operating Systems\r\n");
    esp_printf(putc, "===========================================\r\n\r\n");
    delay(200);

    esp_printf(putc, "Initializing interrupt system...\r\n");
    remap_pic();
    load_gdt();
    init_idt();
    esp_printf(putc, "[OK] IDT & PIC ready\r\n");

    esp_printf(putc, "Enabling interrupts...\r\n");
    asm("sti");
    esp_printf(putc, "[OK] IRQs enabled\r\n\r\n");

    esp_printf(putc, "Type 'help' for commands.\r\n\r\n");
    init_keyboard();   // prints the "$ " prompt

    // idle loop; keyboard IRQs do the work
    while (1) asm("hlt");
}
