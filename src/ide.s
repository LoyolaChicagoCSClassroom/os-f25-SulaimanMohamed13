.intel_syntax noprefix

# ATA read sectors (LBA mode)
# C prototype:
#   void ata_lba_read(unsigned int lba,
#                     unsigned char *buffer,
#                     unsigned int num_sectors);

.globl ata_lba_read
.type ata_lba_read, @function
ata_lba_read:
    # prologue
    push ebp
    mov  ebp, esp

    # save registers we modify
    push eax
    push ebx
    push ecx
    push edx
    push edi

    # args:
    # [ebp+8]  = lba (uint32_t)
    # [ebp+12] = buffer (uint8_t*)
    # [ebp+16] = num_sectors (uint32_t)

    mov  eax, [ebp+8]      # LBA
    mov  edi, [ebp+12]     # buffer pointer
    mov  ecx, [ebp+16]     # sector count
    and  eax, 0x0FFFFFFF   # mask to 28-bit LBA
    mov  ebx, eax          # save LBA in ebx
    mov  esi, ecx          # save remaining sectors in esi

.next_sector:
    cmp  esi, 0
    je   .done             # if no sectors left, finish

    # --- select drive and high LBA bits ---
    mov  edx, 0x03F6       # digital output register
    mov  al,  0x02         # disable drive IRQ
    out  dx, al

    mov  eax, ebx          # LBA in eax
    mov  edx, 0x01F6       # drive/head register
    mov  al, 0xE0          # 1110 0000b: LBA mode, master
    mov  ah, bl
    shr  eax, 24           # high LBA bits in al
    and  al, 0x0F          # keep top 4 bits
    or   al, 0xE0
    out  dx, al

    # --- sector count ---
    mov  edx, 0x01F2       # sector count port
    mov  al, 1             # read 1 sector per loop
    out  dx, al

    # --- LBA low/mid/high ---
    mov  eax, ebx

    mov  edx, 0x01F3       # LBA low (7:0)
    mov  al, bl
    out  dx, al

    mov  edx, 0x01F4       # LBA mid (15:8)
    mov  eax, ebx
    shr  eax, 8
    out  dx, al

    mov  edx, 0x01F5       # LBA high (23:16)
    mov  eax, ebx
    shr  eax, 16
    out  dx, al

    # --- issue READ SECTORS command ---
    mov  edx, 0x01F7       # command / status port
    mov  al,  0x20         # READ SECTORS with retry
    out  dx, al

    # --- wait for BSY=0, DRQ=1 ---
    mov  edx, 0x01F7       # status port
    mov  ecx, 4

.wait_400ns:
    in   al, dx
    test al, 0x80          # BSY bit
    jne  .retry_short
    test al, 0x08          # DRQ bit
    jne  .data_ready
.retry_short:
    dec  ecx
    jg   .wait_400ns

.more_wait:
    in   al, dx
    test al, 0x80          # BSY?
    jne  .more_wait
    test al, 0x21          # ERR or DF?
    jne  .fail

.data_ready:
    # data port
    mov  edx, 0x01F0
    mov  ecx, 256          # 256 words = 512 bytes
    rep  insw              # read 512 bytes into [edi]

    # bump buffer pointer by 512 bytes
    add  edi, 512

    # increment LBA, decrement sector counter
    inc  ebx               # next LBA
    dec  esi               # one sector less
    jmp  .next_sector

.fail:
    mov eax, 1            # return error
    jmp .return

.done:
    mov eax, 0            # return success

.return:
    pop edi
    pop edx
    pop ecx
    pop ebx
    # DO NOT pop eax (we want to keep return value in eax)
    mov esp, ebp
    pop ebp
    ret

