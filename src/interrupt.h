#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <stdint.h>

#define PIC_EOI         0x20
#define PIC1            0x20
#define PIC2            0xA0
#define PIC_1_COMMAND   PIC1
#define PIC_2_COMMAND   PIC2
#define PIC_1_DATA      0x21
#define PIC_2_DATA      0xA1

#define IDT_SIZE 256
#define PIC_1_CTRL 0x20
#define PIC_2_CTRL 0xA0

struct idt_entry {
   uint16_t base_lo;
   uint16_t sel;
   uint8_t  always0;
   uint8_t  flags;
   uint16_t base_hi;
} __attribute__((packed));

struct idt_ptr {
   uint16_t limit;
   uint32_t base;
} __attribute__((packed));

struct eflags {
    unsigned int carry       : 1;
    unsigned int reserved1   : 1;
    unsigned int parity      : 1;
    unsigned int reserved3   : 1;
    unsigned int adjust      : 1;
    unsigned int reserved5   : 1;
    unsigned int zero        : 1;
    unsigned int sign        : 1;
    unsigned int trap        : 1;
    unsigned int interrupt   : 1;
    unsigned int direction   : 1;
    unsigned int overflow    : 1;
    unsigned int iopl        : 2;
    unsigned int nest        : 1;
    unsigned int reserved15  : 1;
    unsigned int resume      : 1;
    unsigned int v8086       : 1;
    unsigned int reserved18  : 14;
}__attribute__((packed));

struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint16_t unused1;
    struct  eflags eflags;
    uint32_t esp;
    uint16_t ss;
    uint16_t unused2;
}__attribute__((packed));

struct seg_desc{
    uint16_t sz;
    uint32_t addr;
} __attribute__((packed));

struct tss_entry {
   uint32_t prev_tss;
   uint32_t esp0;
   uint32_t ss0;
   uint32_t esp1;
   uint32_t ss1;
   uint32_t esp2;
   uint32_t ss2;
   uint32_t cr3;
   uint32_t eip;
   uint32_t eflags;
   uint32_t eax;
   uint32_t ecx;
   uint32_t edx;
   uint32_t ebx;
   uint32_t esp;
   uint32_t ebp;
   uint32_t esi;
   uint32_t edi;
   uint32_t es;
   uint32_t cs;
   uint32_t ss;
   uint32_t ds;
   uint32_t fs;
   uint32_t gs;
   uint32_t ldt;
   uint16_t trap;
   uint16_t iomap_base;
}__attribute__((packed));

struct gdt_entry_bits {
    unsigned int limit_low:16;
    unsigned int base_low : 24;
    unsigned int accessed :1;
    unsigned int read_write :1;
    unsigned int conforming_expand_down :1;
    unsigned int code :1;
    unsigned int always_1 :1;
    unsigned int DPL :2;
    unsigned int present :1;
    unsigned int limit_high :4;
    unsigned int available :1;
    unsigned int always_0 :1;
    unsigned int big :1;
    unsigned int gran :1;
    unsigned int base_high :8;
} __attribute__((packed));

void PIC_sendEOI(unsigned char irq);
void IRQ_clear_mask(unsigned char IRQline);
void IRQ_set_mask(unsigned char IRQline);
void init_idt();
void tss_flush(uint16_t tss);
void load_gdt();
void remap_pic(void);
void outb(uint16_t _port, uint8_t val);
uint8_t inb(uint16_t _port);

#endif
