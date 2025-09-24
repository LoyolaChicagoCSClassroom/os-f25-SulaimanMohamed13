
#include <stdint.h>
#include "rprintf.h"

#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6
#define VIDEO_MEMORY 0xB8000
#define ROWS 25
#define COLS 80

// global cursor position
int cursor = 0;

// Write one character to the screen
void putc(int data) {
    char *video = (char*) VIDEO_MEMORY;

    if (data == '\n') {
        // move to start of next line
        cursor += COLS - (cursor % COLS);
    } else {
        video[cursor * 2] = (char) data;   // ASCII character
        video[cursor * 2 + 1] = 0x07;      // light gray on black
        cursor++;
    }

    // reset cursor if full (later you’ll replace with scrolling)
    if (cursor >= ROWS * COLS) {
        cursor = 0;
    }
}

// port I/O (your code)
uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

// kernel entry
void main() {
    // test printing
    esp_printf(putc, "Hello World!\r\n");

    // your keyboard loop
    while(1) {
        uint8_t status = inb(0x64);
        if(status & 1) {
            uint8_t scancode = inb(0x60);
            // later: translate scancode to char, call putc()
        }
    }
}

