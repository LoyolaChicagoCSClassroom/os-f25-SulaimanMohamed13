#ifndef __IDE_H__
#define __IDE_H__

// ATA LBA read function (implemented in assembly)
// @param lba - Logical Block Address of sector
// @param buffer - Pointer to buffer to store data
// @param numsectors - Number of sectors to read
// @return 0 on success, -1 on failure
int ata_lba_read(unsigned int lba, unsigned char *buffer, unsigned int numsectors);

#endif
