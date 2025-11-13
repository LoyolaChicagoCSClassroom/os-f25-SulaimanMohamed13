#include "fat.h"
#include "ide.h"
#include "rprintf.h"
#include <stdint.h>

// External printf function
extern int putc(int data);
extern void esp_printf(const func_ptr f_ptr, charptr ctrl, ...);

#define rprintf(...) esp_printf(putc, __VA_ARGS__)

// Helper functions
static int strcmp(const char* s1, const char* s2);
static int strncmp(const char* s1, const char* s2, uint32_t n);
static char toupper(char c);
static void* memcpy(void* dest, const void* src, uint32_t n);
static void* memset(void* s, int c, uint32_t n);

// Global variables
static struct boot_sector* bs;
static char bootSector[512];
static char fat_table[8 * SECTOR_SIZE];
static uint32_t root_sector;
static uint32_t data_start_sector;
static uint32_t partition_offset = 2048; // Partition starts at sector 2048

// Initialize FAT filesystem
int fatInit(void) {
    // Read boot sector from partition
    int ret = ata_lba_read(partition_offset, (unsigned char*)bootSector, 1);
rprintf("ata_lba_read returned: %d\r\n", ret);

if (ret != 0) {
    rprintf("Error: Failed to read boot sector\r\n");
    return -1;
}
    
    bs = (struct boot_sector*)bootSector;
    
    // Validate boot signature
    if (bs->boot_signature != 0xAA55) {
        rprintf("Error: Invalid boot signature: 0x%x (expected 0xAA55)\r\n", 
                bs->boot_signature);
        return -1;
    }
    
    // Validate filesystem type
    if (strncmp(bs->fs_type, "FAT12", 5) != 0 && 
        strncmp(bs->fs_type, "FAT16", 5) != 0) {
        rprintf("Error: Invalid filesystem type: %.8s\r\n", bs->fs_type);
        return -1;
    }
    
    // Print filesystem info
    rprintf("FAT Filesystem initialized:\r\n");
    rprintf("  OEM Name: %.8s\r\n", bs->oem_name);
    rprintf("  FS Type: %.8s\r\n", bs->fs_type);
    rprintf("  Bytes per sector: %d\r\n", bs->bytes_per_sector);
    rprintf("  Sectors per cluster: %d\r\n", bs->num_sectors_per_cluster);
    rprintf("  Reserved sectors: %d\r\n", bs->num_reserved_sectors);
    rprintf("  FAT tables: %d\r\n", bs->num_fat_tables);
    rprintf("  Root dir entries: %d\r\n", bs->num_root_dir_entries);
    rprintf("  Sectors per FAT: %d\r\n", bs->num_sectors_per_fat);
    
    // Read FAT table
    uint32_t fat_sector = partition_offset + bs->num_reserved_sectors;
    uint32_t fat_size = bs->num_sectors_per_fat;
    if (fat_size > 8) {
        fat_size = 8; // Limit to our buffer size
    }
    
    if (ata_lba_read(fat_sector, (unsigned char*)fat_table, fat_size) != 0) {
        rprintf("Error: Failed to read FAT table\r\n");
        return -1;
    }
    
    // Calculate root directory sector
    root_sector = partition_offset + bs->num_reserved_sectors + 
                  (bs->num_fat_tables * bs->num_sectors_per_fat);
    
    // Calculate data region start
    uint32_t root_dir_sectors = ((bs->num_root_dir_entries * 32) + 
                                 (bs->bytes_per_sector - 1)) / bs->bytes_per_sector;
    data_start_sector = root_sector + root_dir_sectors;
    
    rprintf("  Root sector: %d\r\n", root_sector);
    rprintf("  Data start sector: %d\r\n", data_start_sector);
    
    return 0;
}

// Extract filename from RDE (based on instructor's code)
static void extract_filename(struct root_directory_entry *rde, char *fname) {
    int k = 0; // index into fname

    // Copy filename (up to 8 chars)
    while(((rde->file_name)[k] != ' ') && (k < 8)) {
        fname[k] = (rde->file_name)[k];
        k++;
    }
    fname[k] = '\0';

    // Check if there's an extension
    if((rde->file_extension)[0] == ' ') { 
        return; // No extension
    }

    // Add extension
    fname[k++] = '.';
    fname[k] = '\0';
    int n = 0;

    while(((rde->file_extension)[n] != ' ') && (n < 3)) {
        fname[k] = (rde->file_extension)[n];
        k++;
        n++;
    }
    fname[k] = '\0';
}

// Open a file
struct file* fatOpen(const char* filename) {
    static struct file fh;
    static char root_dir_buffer[SECTOR_SIZE];
    
    // Convert filename to uppercase and parse
    char name[9] = {0};
    char ext[4] = {0};
    int name_idx = 0, ext_idx = 0;
    int in_ext = 0;
    
    for (int i = 0; filename[i] != '\0' && i < 256; i++) {
        if (filename[i] == '.') {
            in_ext = 1;
            continue;
        }
        if (filename[i] == '/' || filename[i] == '\\') {
            continue;
        }
        
        if (in_ext && ext_idx < 3) {
            ext[ext_idx++] = toupper(filename[i]);
        } else if (!in_ext && name_idx < 8) {
            name[name_idx++] = toupper(filename[i]);
        }
    }
    
    // Pad with spaces
    while (name_idx < 8) name[name_idx++] = ' ';
    while (ext_idx < 3) ext[ext_idx++] = ' ';
    
    rprintf("Looking for file: '%.8s.%.3s'\r\n", name, ext);
    
    // Calculate number of sectors in root directory
    uint32_t root_dir_sectors = ((bs->num_root_dir_entries * 32) + 
                                 (bs->bytes_per_sector - 1)) / bs->bytes_per_sector;
    
    // Search root directory
    for (uint32_t sector = 0; sector < root_dir_sectors; sector++) {
        if (ata_lba_read(root_sector + sector, (unsigned char*)root_dir_buffer, 1) != 0) {
            rprintf("Error: Failed to read root directory sector %d\r\n", sector);
            return 0;
        }
        
        struct root_directory_entry* entries = (struct root_directory_entry*)root_dir_buffer;
        uint32_t entries_per_sector = SECTOR_SIZE / sizeof(struct root_directory_entry);
        
        for (uint32_t i = 0; i < entries_per_sector; i++) {
            // Check for end of directory
            if (entries[i].file_name[0] == 0x00) {
                rprintf("Error: File not found (end of directory)\r\n");
                return 0;
            }
            
            // Skip deleted entries
            if ((uint8_t)entries[i].file_name[0] == 0xE5) {
                continue;
            }
            
            // Skip volume labels
            if (entries[i].attribute & 0x08) {
                continue;
            }
            
            // Compare name and extension
            if (strncmp(entries[i].file_name, name, 8) == 0 &&
                strncmp(entries[i].file_extension, ext, 3) == 0) {
                
                char temp_name[16];
                extract_filename(&entries[i], temp_name);
                rprintf("Found file: %s\r\n", temp_name);
                rprintf("  Size: %d bytes\r\n", entries[i].file_size);
                rprintf("  Cluster: %d\r\n", entries[i].cluster);
                
                // Initialize file handle
                memcpy(&fh.rde, &entries[i], sizeof(struct root_directory_entry));
                fh.position = 0;
                fh.current_cluster = entries[i].cluster;
                fh.start_cluster = entries[i].cluster;
                
                return &fh;
            }
        }
    }
    
    rprintf("Error: File not found\r\n");
    return 0;
}

// Get next cluster from FAT
static uint16_t get_next_cluster(uint16_t cluster) {
    if (strncmp(bs->fs_type, "FAT12", 5) == 0) {
        // FAT12: 1.5 bytes per entry
        uint32_t fat_offset = cluster + (cluster / 2);
        uint16_t* fat = (uint16_t*)fat_table;
        uint16_t entry = fat[fat_offset / 2];
        
        if (cluster & 1) {
            entry >>= 4;
        } else {
            entry &= 0x0FFF;
        }
        
        if (entry >= 0xFF8) return 0xFFFF; // End of chain
        return entry;
    } else {
        // FAT16: 2 bytes per entry
        uint16_t* fat = (uint16_t*)fat_table;
        uint16_t entry = fat[cluster];
        
        if (entry >= 0xFFF8) return 0xFFFF; // End of chain
        return entry;
    }
}

// Read from file
int fatRead(struct file* fh, void* buffer, uint32_t size) {
    if (!fh) return -1;
    
    uint32_t bytes_read = 0;
    uint8_t* buf = (uint8_t*)buffer;
    static char cluster_buffer[SECTOR_SIZE];
    
    uint32_t bytes_per_cluster = bs->bytes_per_sector * bs->num_sectors_per_cluster;
    
    while (bytes_read < size && fh->position < fh->rde.file_size) {
        // Calculate position within current cluster
        uint32_t cluster_offset = fh->position % bytes_per_cluster;
        uint32_t bytes_remaining_in_cluster = bytes_per_cluster - cluster_offset;
        uint32_t bytes_remaining_in_file = fh->rde.file_size - fh->position;
        uint32_t bytes_to_read = size - bytes_read;
        
        if (bytes_to_read > bytes_remaining_in_cluster) {
            bytes_to_read = bytes_remaining_in_cluster;
        }
        if (bytes_to_read > bytes_remaining_in_file) {
            bytes_to_read = bytes_remaining_in_file;
        }
        
        // Calculate sector number (cluster 2 is the first data cluster)
        uint32_t sector = data_start_sector + 
                         ((fh->current_cluster - 2) * bs->num_sectors_per_cluster);
        
        // Read sector
        if (ata_lba_read(sector, (unsigned char*)cluster_buffer, bs->num_sectors_per_cluster) != 0) {
            rprintf("Error: Failed to read data cluster\r\n");
            return -1;
        }
        
        // Copy data to buffer
        memcpy(buf + bytes_read, cluster_buffer + cluster_offset, bytes_to_read);
        
        bytes_read += bytes_to_read;
        fh->position += bytes_to_read;
        
        // Move to next cluster if needed
        if (cluster_offset + bytes_to_read >= bytes_per_cluster) {
            uint16_t next = get_next_cluster(fh->current_cluster);
            if (next == 0xFFFF) {
                break; // End of file
            }
            fh->current_cluster = next;
        }
    }
    
    return bytes_read;
}

// Helper function implementations
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int strncmp(const char* s1, const char* s2, uint32_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static char toupper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

static void* memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

static void* memset(void* s, int c, uint32_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}
