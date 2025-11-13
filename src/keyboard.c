#include <stdint.h>
#include "keyboard.h"
#include "rprintf.h"
#include "fat.h"

extern int putc(int data);

// --- maps ---
unsigned char keyboard_map[128] = {
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',
 '9', '0', '-', '=', '\b', '\t',
 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
   0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
'\'', '`',   0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
 'm', ',', '.', '/',   0, '*',
   0, ' ',  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  0,  0,  0,  0,  0, '-',  0,  0,  0, '+',
   0,  0,  0,  0,  0,   0,   0,  0,  0,  0,
};

unsigned char keyboard_map_shift[128] = {
   0,  27, '!', '@', '#', '$', '%', '^', '&', '*',
 '(', ')', '_', '+', '\b', '\t',
 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
   0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
 '"', '~',   0, '|', 'Z', 'X', 'C', 'V', 'B', 'N',
 'M', '<', '>', '?',   0, '*',
   0, ' ',  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  0,  0,  0,  0,  0, '-',  0,  0,  0, '+',
   0,  0,  0,  0,  0,   0,   0,  0,  0,  0,
};

// --- tiny stdlib replacements (freestanding) ---
static int my_strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static void my_puts(const char* s) {
    while (*s) putc(*s++);
}

// --- command buffer + shift state ---
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];
static int  cmd_index = 0;
static int  shift_pressed = 0;

static void process_command(void) {
    cmd_buffer[cmd_index] = '\0';

    if (cmd_index == 0) {
        my_puts("$ ");
        return;
    }

    if (my_strcmp(cmd_buffer, "help") == 0) {
        my_puts("\r\nAvailable commands:\r\n");
        my_puts("  help  - Show this help message\r\n");
        my_puts("  clear - Clear the screen (poor-man)\r\n");
        my_puts("  echo  - Echo test message\r\n");
        my_puts("  about - About this OS\r\n");
        my_puts("  time  - Show uptime message\r\n");
        my_puts("  fat   - Test FAT filesystem\r\n");
    } else if (my_strcmp(cmd_buffer, "clear") == 0) {
        for (int i = 0; i < 25; i++) my_puts("\r\n");
        my_puts("Screen cleared!\r\n");
    } else if (my_strcmp(cmd_buffer, "echo") == 0) {
        my_puts("\r\nHello from your OS!\r\n");
    } else if (my_strcmp(cmd_buffer, "about") == 0) {
        my_puts("\r\nCustom OS - COMP 310 Project\r\n");
        my_puts("Interrupt-driven keyboard handler\r\n");
        my_puts("Built with love and assembly!\r\n");
    } else if (my_strcmp(cmd_buffer, "time") == 0) {
        my_puts("\r\nSystem has been running since boot.\r\n");
        my_puts("Uptime: Unknown (no timer yet)\r\n");
    } else if (my_strcmp(cmd_buffer, "fat") == 0) {
        my_puts("\r\n=== FAT Filesystem Test ===\r\n");
        
        my_puts("Initializing FAT filesystem...\r\n");
        if (fatInit() != 0) {
            my_puts("[ERROR] Failed to initialize!\r\n");
        } else {
            my_puts("[OK] FAT initialized\r\n");
            
            my_puts("Opening 'testfile.txt'...\r\n");
            struct file* fh = fatOpen("testfile.txt");
            if (!fh) {
                my_puts("[ERROR] File not found!\r\n");
            } else {
                static char buffer[256];
                int bytes = fatRead(fh, buffer, sizeof(buffer) - 1);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    my_puts("File contents:\r\n");
                    my_puts(buffer);
                    my_puts("\r\n[OK] Read ");
                    esp_printf(putc, "%d", bytes);
                    my_puts(" bytes\r\n");
                } else {
                    my_puts("[ERROR] Read failed!\r\n");
                }
            }
        }
    } else {
        my_puts("\r\nUnknown command: ");
        my_puts(cmd_buffer);
        my_puts("\r\nType 'help' for available commands.\r\n");
    }

    cmd_index = 0;
    my_puts("$ ");
}

void handle_keyboard_input(uint8_t scancode) {
    // Left/Right Shift press
    if (scancode == 0x2A || scancode == 0x36) { shift_pressed = 1; return; }
    // Left/Right Shift release (0x2A|0x36 + 0x80)
    if (scancode == 0xAA || scancode == 0xB6) { shift_pressed = 0; return; }

    // Only handle key-down
    if (scancode >= 0x80) return;

    char ch = shift_pressed ? keyboard_map_shift[scancode]
                            : keyboard_map[scancode];

    if (ch == '\b') {
        if (cmd_index > 0) {
            cmd_index--;
            // erase the char on screen: back, space, back
            putc('\b'); putc(' '); putc('\b');
        }
    } else if (ch == '\n') {
        // CRLF then process
        putc('\r'); putc('\n');
        process_command();
    } else if (ch != 0) {
        if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = ch;
            putc(ch);
        }
    }
}

void init_keyboard(void) {
    cmd_index = 0;
    shift_pressed = 0;
    my_puts("$ ");
}
