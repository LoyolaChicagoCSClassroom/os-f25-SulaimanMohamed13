ENTRY(_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
    /* Begin at 1 MiB, conventional place for kernels */
    . = 1M;
    
    /* Multiboot header must be in first 8KB */
    .multiboot : ALIGN(4) {
        *(.multiboot)
	*(.text)
    }
    
    .rodata : ALIGN(4K) {
        *(.rodata)
    }
    
    .data : ALIGN(4K) {
        start_data = .;
        *(.data)
        end_data = .;
    }
    
    .bss : ALIGN(4K) {
        start_bss = .;
        *(.bss)
        *(COMMON)
        end_bss = .;
    }
    
    end_kernel = .;
}
