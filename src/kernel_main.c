#include <stdint.h>
#include "rprintf.h"
#include "interrupt.h"
#include "keyboard.h"
#include "fat.h"

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
        if (cur_col > 0) {
            cur_col--;
            vga_put_at(' ', cur_row, cur_col);
        } else if (cur_row > 0) {
            cur_row--; cur_col = VGA_W - 1;
            vga_put_at(' ', cur_row, cur_col);
        }
    } else {
        vga_put_at(ch, cur_row, cur_col);
        cur_col++;
        if (cur_col >= VGA_W) { cur_col = 0; cur_row++; }
    }

    scroll_if_needed();
    return 0;
}

static void delay(int ms) { for (volatile int i = 0; i < ms * 100000; i++) {} }

// FAT filesystem test command
void test_fat_filesystem(void) {
    esp_printf(putc, "\r\n=== FAT Filesystem Test ===\r\n");
    
    esp_printf(putc, "Initializing FAT filesystem...\r\n");
    if (fatInit() != 0) {
        esp_printf(putc, "[ERROR] Failed to initialize FAT filesystem!\r\n");
        return;
    }
    
    esp_printf(putc, "\r\nOpening test file 'testfile.txt'...\r\n");
    
    struct file* fh = fatOpen("testfile.txt");
    if (!fh) {
        esp_printf(putc, "[ERROR] Failed to open file!\r\n");
        return;
    }
    
    static char buffer[512];
    
    esp_printf(putc, "Reading file contents...\r\n");
    
    int bytes_read = fatRead(fh, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {
        esp_printf(putc, "[ERROR] Failed to read file!\r\n");
        return;
    }
    
    buffer[bytes_read] = '\0';
    
    esp_printf(putc, "\r\n--- File Contents (%d bytes) ---\r\n", bytes_read);
    esp_printf(putc, "%s\r\n", buffer);
    esp_printf(putc, "--- End of File ---\r\n");
    
    esp_printf(putc, "\r\n[OK] FAT filesystem test completed!\r\n");
}

void kernel_main(void) {
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

    test_fat_filesystem();
    esp_printf(putc, "\r\n");

    esp_printf(putc, "Type 'help' for commands.\r\n\r\n");
    init_keyboard();

    while (1) asm("hlt");
}
