#ifndef GB_DISK_H
#define GB_DISK_H

#include <stdint.h>

// The file entries of all the file indexes
extern uint32_t file_lba_indexes[30];
// whether host does safe-eject
static uint8_t ejected = 0;

// Metadata about the fake disk and its structure
enum
{
  ROOT_DIR_ENTRY_SIZE = 32,
  ROOT_DIR_SIZE_OFFS = 28,
  ROOT_DIR_CLST_OFFS  = 26,
  ROOT_DIR_STATUS_ENTRY = 1,
  DISK_BLOCK_COUNT  = 0x200000, // Pretend to be a 1GB drive
  BLOCK_SIZE = 512,
  CLUSTER_BLOCK_SIZE = 8,
  CLUSTER_BYTE_SIZE = BLOCK_SIZE * CLUSTER_BLOCK_SIZE, // Assuming block and cluster size stay standard (512 and 8 respectfully) this is 4k
  // Hold enough entry for every file I want
  // 1 entries for status.txt
  // 8192 entries for 32MB ROM
  // 8192 entries for 32MB SRAM
  // 2 entries (8K) for each photo, 32 photos total
  // Two bytes per entry (FAT16 = 16 bit entries)
  // Add 382 to make it block aligned (divisible by 512)
  FAT_TABLE_BYTE_SIZE = ((1 + 8192 + 8192 + (2 * 32)) * 2) + 382, 
  // I am not actually holding the whole FAT table in RAM, so need to know when 
  // the PC requests something beyond that so I can just send it a 0
  FAT_TABLE_BLOCK_SIZE = FAT_TABLE_BYTE_SIZE / BLOCK_SIZE,
  // Root directory should be large enough to hold every file needed
  // 1 status file
  // 1 ROM file
  // 1 SRAM file
  // 30 Photo files
  // 32 bytes per entry
  // Add 480 bytes to make it block aligned (divisible by 512)
  BYTE_SIZE_ROOT_DIRECTORY = ((1 + 1 + 1 + 30) * 32) + 480, 
  BLOCK_SIZE_ROOT_DIRECTORY = BYTE_SIZE_ROOT_DIRECTORY / BLOCK_SIZE,
  STATUS_FILE_SIZE = BLOCK_SIZE, // Small for now, can be up to 1 cluster (4k) with current layout
};

// Indexes of all the LBA starting points
// Used in conjunction with file_lba_indexes
enum {
  FILE_INDEX_RESERVED          = 0,  // First cluster is for all the FAT magic data
  FILE_INDEX_FAT_TABLE_1_START = 1,  // FAT table starts at 0x1, 0x81 blocks in size
  FILE_INDEX_FAT_TABLE_2_START = 2,  // Redundant FAT table is 0x81 blocks later
  FILE_INDEX_ROOT_DIRECTORY    = 3,  // Root directory starts after second FAT table
  FILE_INDEX_DATA_STARTS       = 4,  // Root directory is 0x20 blocks (32 * 512 = 16384 bytes). Can store 16384 / 32 = 512 files total
  // Status file is the first one in the data section
  FILE_INDEX_STATUS_FILE       = 5,  
  // ROM bin starts after status file (status file 1 cluster large, 8 blocks)
  FILE_INDEX_ROM_BIN           = 6,
  // SRAM bin starts after ROM bin (ROM bin 8192 clusters large, 65536 blocks, 32 MB total filesize)
  FILE_INDEX_SRAM_BIN          = 7,
  // Photos starts after SRAM bin (SRAM bin 8192 clusters large, 65536 blocks, 32 MB total filesize)
  FILE_INDEX_PHOTOS_START      = 8,
  // Photos end after 30 entries
  FILE_INDEX_PHOTOS_END        = 9,
  // End of the files on the drive
  FILE_INDEX_DATA_END          = 10  
};

// Arrays that hold the fake disk data
extern uint8_t DISK_reservedSection[BLOCK_SIZE];
extern uint8_t DISK_fatTable[FAT_TABLE_BYTE_SIZE];
extern uint8_t DISK_rootDirectory[BYTE_SIZE_ROOT_DIRECTORY];
extern uint8_t DISK_status_file[STATUS_FILE_SIZE];

// Set up the memory needed for the fake disk
void init_disk_mem();
// Initialize the fake disk
void init_disk();
// Append some data to the status file, const str
void append_status_file(const uint8_t* buf);
// Append data to the status file, arbitrary buf
void append_status_file_buf(uint8_t* buf);
#endif