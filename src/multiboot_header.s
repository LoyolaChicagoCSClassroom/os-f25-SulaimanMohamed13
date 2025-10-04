# Multiboot v1 header (GRUB loads us)
.set ALIGN,    1<<0            # align modules on page boundaries
.set MEMINFO,  1<<1            # request memory map
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

# Entry point
.section .text
.global _start
.type _start, @function

_start:
    # Set up stack
    mov $stack_top, %esp
    
    # Call kernel_main
    call kernel_main
    
    # Hang if kernel_main returns
halt:
    cli
    hlt
    jmp halt

# Stack space
.section .bss
.align 16
stack_bottom:
.skip 16384  # 16 KB stack
stack_top:
