#include <stdint.h>
#include "rprintf.h"

/* Port I/O helper */
uint8_t inb(uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(rv) : "dN"(_port));
    return rv;
}

/* VGA text mode */
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
        cur_row++;
        cur_col = 0;
    } else if (ch == '\r') {
        cur_col = 0;
    } else {
        vga_put_at(ch, cur_row, cur_col++);
        if (cur_col >= VGA_W) {
            cur_col = 0;
            cur_row++;
        }
    }
    scroll_if_needed();
    return 0;
}

void delay(int milliseconds) {
    for (volatile int i = 0; i < milliseconds * 100000; i++) {
        // Busy wait
    }
}

/* kernel entry */
void kernel_main() {
    // Clear screen
    for (int r = 0; r < VGA_H; ++r)
        for (int c = 0; c < VGA_W; ++c)
            VGA[r * VGA_W + c] = ((uint16_t)VGA_COLOR << 8) | ' ';
    
    // Test output with delays
    esp_printf(putc, "Hello World!\r\n");
    delay(500);
    
    esp_printf(putc, "OS Boot Successful!\r\n");
    delay(500);
    
    esp_printf(putc, "Current Execution Level: Kernel Mode\r\n");
    delay(500);
    
    // Print 25 lines with delay to see scrolling
    for (int i = 1; i <= 25; i++) {
        esp_printf(putc, "Test line %d\r\n", i);
        delay(300);
    }
    
    // Keyboard loop
    while(1) {
        uint8_t status = inb(0x64);
        if(status & 1) {
            uint8_t scancode = inb(0x60);
        }
    }
}
