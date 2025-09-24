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
