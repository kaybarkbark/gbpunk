#include "gb_disk.h"
#include "cart.h"
#include "msc_disk.h"
#include "mappers/gbcam.h"
#include "gb.h"

#include <string.h>
#include <stdio.h>


/*  - Private #defines -  */
#define ROOT_DIR_ENTRY(X)     X*ROOT_DIR_ENTRY_SIZE
// Convert cluster size to blocks size
#define CLS2BLK(x)  (x*CLUSTER_BLOCK_SIZE)
// Convert block size to byte size
#define BLK2BYTE(x) (x*BLOCK_SIZE)
// Convert cluster size to block size
#define CLS2BYTE(x) BLK2BYTE(CLS2BLK(x))

// Indexes of all the cluster sizes for the files
// Used in conjunction with file_cluster_sizes
enum {
  INDEX_CLUSTER_SIZE_STATUS_FILE  = 0,
  INDEX_CLUSTER_SIZE_ROM_FILE     = 1,
  INDEX_CLUSTER_SIZE_RAM_FILE     = 2,
  INDEX_CLUSTER_SIZE_PHOTOS       = 3
};

// Indexes of all the cluster starting points. This is not redundant, as
// they're just calculated from the LBA starting points. A second lookup table
// is faster and makes the disk more responsive.
// Used in conjunction with file_starting_clusters
enum {
  INDEX_CLUSTER_START_FS = 0,
  INDEX_CLUSTER_START_STATUS_FILE = 1,
  INDEX_CLUSTER_START_ROM_FILE = 2,
  INDEX_CLUSTER_START_RAM_FILE = 3,
  INDEX_CLUSTER_START_PHOTOS = 4
};  

/*  - Private Variables -  */
// The most recent root directory entry
uint32_t latest_rd_entry = 0;
// The most recent FAT table entry byte
uint32_t latest_fat_entry = 0;
// The file entries of all the file indexes
uint32_t file_lba_indexes[30] = {0};
// The cluster sizes of all the files
uint32_t file_cluster_sizes[4] = {0};
// The starting clusters of all the files
uint32_t file_starting_clusters[5] = {0};
// The size of the status file
uint16_t status_file_size = 0;
// A blank root directory entry to use
uint8_t blank_rd_entry[] = {      
  ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , // filename (uninitialized)
  0x20, 0x00, 0xC6, 0x52, 0x6D, 0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, // Bunch of unknown options
  0x00, 0x00, //cluster location (unititialized)
  0x00, 0x00, 0x00, 0x00, // Filesize in bytes (unititialized)}
};

void append_status_file(const uint8_t* buf){
  uint16_t buf_index = 0;
  for(;;){
    if(status_file_size > STATUS_FILE_SIZE - 1){
      return;
    }
    if(buf[buf_index] == '\0'){
      return;
    }
    DISK_status_file[status_file_size] = buf[buf_index];
    status_file_size++;
    buf_index++;
  }
}
void append_status_file_buf(uint8_t* buf){
  uint16_t buf_index = 0;
  for(;;){
    if(status_file_size > STATUS_FILE_SIZE - 1){
      return;
    }
    if(buf[buf_index] == '\0'){
      return;
    }
    DISK_status_file[status_file_size] = buf[buf_index];
    status_file_size++;
    buf_index++;
  }
}

void init_disk_mem(){
  memset(DISK_status_file, ' ', STATUS_FILE_SIZE);
  // memset(DISK_fatTable, 0xFF, FAT_TABLE_SIZE);
}

// Full reserved section of the disk, containing all the FAT magic
uint8_t DISK_reservedSection[BLOCK_SIZE] = 
{
    0xEB, 0x3C, 0x90,                               // ASM instructions for booting
    0x4D, 0x53, 0x57, 0x49, 0x4E, 0x34, 0x2E, 0x31, // MSWIN4.1 (TODO: Change this to something cooler? Fuck microsoft)
    0x00, 0x02,                                     // 512 byte block size (0x200)
    0x08,                                           // 8 blocks per cluster ( 6 * 512 = 4096 = 4K)
    0x01, 0x00,                                     // BPB_RsvdSecCnt: 1 reserved block
    0x02,                                           // Number of fat tables: 2 (for redundancy and compatibility)
    0x00, 0x02,                                     // BPB_RootEntCnt: Max number of files in root dir (0x200)
    0x00, 0x00,                                     // Blocks in the FS: Set to 0, since doesn't fit in 2 bytes
    0xF8,                                           // Disk type: 0xF8 = non removable, 0xF0 = removable (TODO: change to 0xF0)
    0x81, 0x00,                                     // BPB_FATSz16 - Size of fat table (0x81 blocks, 66048 bytes, can hold 33024 clusters)
    0x01, 0x00,                                     // BPB_SecPerTrk: 1, this is not spinning platter drive
    0x01, 0x00,                                     // BPB_NumHeads: 1, this is not a spinning platter drive
    0x01, 0x00, 0x00, 0x00,                         // BPB_HiddSec: Blocks before start partition (???)
    0xFF, 0xFF, 0x03, 0x00,                         // BPB_TotSec32: Blocks in filesystem (0x3FFFF = 262143 blocks = 134217216 bytes = 1GB)
    0x00,                                           // BS_DrvNum: Low level disk service drive number (don't care, not a real drive)
    0x00,                                           // Reserved
    0x29,                                           // Use extended boot fields (volume serial number, volume label, file system type)
    0x50, 0x04, 0x0B, 0x00,                         // Volume Serial Number
    'G' , 'B' , 'P' , 'U' , 'N' , 'K' ,             // Volume Label
    ' ' , ' ' , ' ' , ' ' , ' ' ,  
    'F', 'A', 'T', '1', '6', ' ', ' ', ' ',         // File system type
    0x00, 0x00,                                     // Zero up to 2 last bytes of boot sector
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x55, 0xAA // FAT Signature
};
// The actual FAT table
uint8_t DISK_fatTable[FAT_TABLE_BYTE_SIZE] ={0};
// The FAT root directory for all files
uint8_t DISK_rootDirectory[BYTE_SIZE_ROOT_DIRECTORY] = 
{
      // first entry is volume label
      ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , 0x28, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // We will fill in all other entries later
};

uint8_t DISK_status_file[STATUS_FILE_SIZE] = {0};

/*  - Private Function Declarations -  */

// Convert byte size to blocks required to hold those bytes
uint32_t byte2blk(uint32_t numbytes); 
// Convert byte size to number of clusters required to hold those bytes
uint32_t byte2cls(uint32_t numbytes);
// Set the filesize of a file in the root directory
void rd_set_file_size(uint32_t entry, uint32_t filesize);
// Reset the MCU
void software_reset();
// Set up all amount of clusters needed for each file
void set_file_cluster_sizes();
// Set up the starting logical block addresses for each file
void set_file_lba_indexes();
// Set up the starting clusters for each file
void set_starting_clusters();
// Append some data to the status file, const str
void append_status_file(const uint8_t* buf);
// Append data to the status file, arbitrary buf
void append_status_file_buf(uint8_t* buf);
// Set up the memory needed for the fake disk
void init_disk_mem();
// Set the file size of a file in the root directory
void rd_set_file_size(uint32_t entry, uint32_t filesize);
// Set the starting cluster for a given file 
void rd_set_cluster_start(uint32_t entry, uint16_t start_cluster);
// Set the file name of the file in the root directory
void rd_set_file_name(uint32_t entry, uint8_t* name, uint16_t namelen, const char* ext);
// Append a new entry to the root directory
void rd_append_new_entry();
// Build a cluster chain in the FAT table for a new file. Return the new most recent cluster after building the chain
uint32_t fat_build_cluster_chain(uint32_t starting_cluster, uint32_t num_bytes);
// Add a new file to the fake disk
void append_new_file(uint8_t* name, uint16_t namelen, const char* ext, uint32_t filesize, uint32_t fat_entry);
// Initialize the disk
void init_disk();

/*  - Private Function Definitions -  */

uint32_t byte2blk(uint32_t numbytes){
  uint32_t blks = 0;
  if(numbytes >= BLOCK_SIZE){
    blks = numbytes / BLOCK_SIZE;
  }
  // If there is remainder, we need an extra block
  if(numbytes % BLOCK_SIZE){
    blks++;
  }
  return blks;
}

// Convert byte size to number of clusters required
uint32_t byte2cls(uint32_t numbytes){
  uint32_t clus = 0;
  if(numbytes >= CLUSTER_BYTE_SIZE){
    clus = numbytes / CLUSTER_BYTE_SIZE;
  }
  // If there is remainder, we need an extra block
  if(numbytes % CLUSTER_BYTE_SIZE){
    clus++;
  }
  return clus;
}

void set_file_cluster_sizes(){
  // Hardcoded, just one cluster large for now
  file_cluster_sizes[INDEX_CLUSTER_SIZE_STATUS_FILE] = 1;
  file_cluster_sizes[INDEX_CLUSTER_SIZE_ROM_FILE] = byte2cls(the_cart.rom_size_bytes);
  file_cluster_sizes[INDEX_CLUSTER_SIZE_RAM_FILE] = byte2cls(the_cart.ram_size_bytes);
  // Determined emprically
  file_cluster_sizes[INDEX_CLUSTER_SIZE_PHOTOS] = 2;
}

void set_file_lba_indexes(){
  // First cluster is for all the FAT magic data
  file_lba_indexes[FILE_INDEX_RESERVED]               = 0x00000;
  // FAT table starts at 0x1, 0x81 blocks in size
  file_lba_indexes[FILE_INDEX_FAT_TABLE_1_START]      = 0x00001;
  // Redundant FAT table is 0x81 blocks later
  file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START]      = 0x00082;
  // Root directory starts after second FAT table
  file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY]         = 0x00103;
  // Root directory is 0x20 blocks (32 * 512 = 16384 bytes). Can store 16384 / 32 = 512 files total
  file_lba_indexes[FILE_INDEX_DATA_STARTS]            = 0x00123;
  file_lba_indexes[FILE_INDEX_STATUS_FILE]            = file_lba_indexes[FILE_INDEX_DATA_STARTS];
  file_lba_indexes[FILE_INDEX_ROM_BIN]                = file_lba_indexes[FILE_INDEX_STATUS_FILE] + CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_STATUS_FILE]);
  file_lba_indexes[FILE_INDEX_SRAM_BIN]               = file_lba_indexes[FILE_INDEX_ROM_BIN] + CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_ROM_FILE]);
  uint32_t photo_cluster_size = 0;
  if(the_cart.mapper_type == MAPPER_GBCAM){
    photo_cluster_size = file_cluster_sizes[INDEX_CLUSTER_SIZE_PHOTOS];
  }
  file_lba_indexes[FILE_INDEX_PHOTOS_START]           = file_lba_indexes[FILE_INDEX_SRAM_BIN] + CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_RAM_FILE]);
  file_lba_indexes[FILE_INDEX_PHOTOS_END]             = file_lba_indexes[FILE_INDEX_PHOTOS_START] + (CLS2BLK(photo_cluster_size) * 30);
  file_lba_indexes[FILE_INDEX_DATA_END]               = file_lba_indexes[FILE_INDEX_PHOTOS_END];
}

void set_starting_clusters(){
  file_starting_clusters[INDEX_CLUSTER_START_FS] = 2;
  file_starting_clusters[INDEX_CLUSTER_START_STATUS_FILE] = file_starting_clusters[INDEX_CLUSTER_START_FS];
  file_starting_clusters[INDEX_CLUSTER_START_ROM_FILE] = file_starting_clusters[INDEX_CLUSTER_START_STATUS_FILE] + file_cluster_sizes[INDEX_CLUSTER_SIZE_STATUS_FILE];
  file_starting_clusters[INDEX_CLUSTER_START_RAM_FILE] = file_starting_clusters[INDEX_CLUSTER_START_ROM_FILE] + file_cluster_sizes[INDEX_CLUSTER_SIZE_ROM_FILE];
  file_starting_clusters[INDEX_CLUSTER_START_PHOTOS] = file_starting_clusters[INDEX_CLUSTER_START_RAM_FILE] + file_cluster_sizes[INDEX_CLUSTER_SIZE_RAM_FILE];
}

// Set the file size of a file in the root directory
void rd_set_file_size(uint32_t entry, uint32_t filesize){
  for(uint8_t i = 0; i < 4; i++){
    DISK_rootDirectory[ROOT_DIR_ENTRY(entry) + ROOT_DIR_SIZE_OFFS + i] = (filesize & (0xFF << (i * 8))) >> i * 8;
  }
}

// Set the starting cluster for a given file 
void rd_set_cluster_start(uint32_t entry, uint16_t start_cluster){
  for(uint8_t i = 0; i < 2; i++){
    DISK_rootDirectory[ROOT_DIR_ENTRY(entry) + ROOT_DIR_CLST_OFFS + i] = (start_cluster & (0xFF << (i * 8))) >> i * 8;
  }
}

// Set the file name of the file in the root directory
void rd_set_file_name(uint32_t entry, uint8_t* name, uint16_t namelen, const char* ext){
  memcpy(DISK_rootDirectory + ROOT_DIR_ENTRY(entry), name, namelen);
  memcpy(DISK_rootDirectory + ROOT_DIR_ENTRY(entry) + 8, ext, 3);
}

// Append a new entry to the root directory
void rd_append_new_entry(){
  latest_rd_entry++;
  memcpy(DISK_rootDirectory + latest_rd_entry * ROOT_DIR_ENTRY_SIZE, blank_rd_entry, ROOT_DIR_ENTRY_SIZE);
}

// Build a cluster chain in the FAT table for a new file. Return the new most recent cluster after building the chain
uint32_t fat_build_cluster_chain(uint32_t starting_cluster, uint32_t num_bytes){
  // Calculate the number of clusters needed
  uint16_t num_clusters = num_bytes / CLUSTER_BYTE_SIZE;
  // If there's a remainder, we need another cluster
  if(num_bytes % CLUSTER_BYTE_SIZE){
    num_clusters++;
  }
  // Need to build up a cluster chain if the file does not fit in one cluster
  if(num_clusters > 1){
    for(uint16_t i = 0; i < num_clusters; i++){
      uint16_t next_entry = starting_cluster + 1;
      DISK_fatTable[starting_cluster * 2] = next_entry & 0xFF;
      DISK_fatTable[(starting_cluster * 2) + 1] = (next_entry & 0xFF00) >> 8;
      starting_cluster++;
    }
    starting_cluster--;
  }
  // Last cluster is EOF
    // TODO: Remove if this breaks things
  DISK_fatTable[(starting_cluster * 2)] = 0xFF;
  DISK_fatTable[(starting_cluster * 2) + 1] = 0xFF;
  starting_cluster+=2;
  return starting_cluster; // TODO: Not sure how accurate this is, make sure it works
}

void append_new_file(uint8_t* name, uint16_t namelen, const char* ext, uint32_t filesize, uint32_t fat_entry){
  // Append a new blank entry to the root directory so we can populate it
  rd_append_new_entry();
  // Set the file names
  rd_set_file_name(latest_rd_entry, name, namelen, ext);
  // Set the starting cluster
  rd_set_cluster_start(latest_rd_entry, fat_entry);
  // Set the filesize
  rd_set_file_size(latest_rd_entry, filesize);
  // Starting at first cluster, populate the FAT chain
  fat_build_cluster_chain(fat_entry, filesize);
}

void init_disk(){
  // Set the cluster sizes of all the files
  set_file_cluster_sizes();
  // Set the file indexes
  set_file_lba_indexes();
  // Set the starting clusters of all the files
  set_starting_clusters();

  // HANDLE VOLUME INFO
  // First, initialize the names for everything
  // Set the volume label (entry 0) to the cart name (first 8 chars)
  rd_set_file_name(0, the_cart.title, 8, "   ");

  // HANDLE FAT TABLE
  // Set first two entries to 0xFF, as per standard
  for(uint8_t i = 0; i < 4; i++){
    DISK_fatTable[i] = 0xFF;
    latest_fat_entry++;
  }

  // HANDLE STATUS FILE
  // Don't move this out of order! We rely on the status file 
  // being the first entry in the root directory
  // Create the status file
  // Set name (6 chars), extension, filesize, starting cluster 2
  char status_name[] = {"status"};
  append_new_file(status_name, 6, "txt", status_file_size, file_starting_clusters[INDEX_CLUSTER_START_STATUS_FILE]);

  // HANDLE ROM INFO
  // Create the ROM file
  // Set name (8 chars), extension, filesize, starting cluster 3
  append_new_file(the_cart.title, 8, "bin", the_cart.rom_size_bytes, file_starting_clusters[INDEX_CLUSTER_START_ROM_FILE]);
  
  // HANDLE RAM INFO
  // If RAM exists, deal with it
  if(the_cart.ram_size_bytes){
    // Create the RAM file
    // Set name (8 chars), extension, filesize, starting cluster 8195
    append_new_file(the_cart.title, 8, "sav", the_cart.ram_size_bytes, file_starting_clusters[INDEX_CLUSTER_START_RAM_FILE]);
  }
  // Otherwise, zero out the root directory for it
  else{
    memset(DISK_rootDirectory + 96, 0, 32);
  }
  if(the_cart.mapper_type == MAPPER_GBCAM){
    for(uint8_t i = 0; i < 30; i++){
      // Note: RD starts at 0x00020620 when debugging these
      char name[9] = {0};
      snprintf(name, 9, "GBCAM_%i", i);
      append_new_file(name, 8, "bmp", 7286, file_starting_clusters[INDEX_CLUSTER_START_PHOTOS] + (i * file_cluster_sizes[INDEX_CLUSTER_SIZE_PHOTOS]));
    }
  }
}


// Links for FAT:
// https://www.pjrc.com/tech/8051/ide/fat32.html
// http://www.c-jump.com/CIS24/Slides/FAT/FAT.html
// http://www.tavi.co.uk/phobos/fat.html
// https://cpl.li/posts/2019-03-12-mbrfat/
// https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html
// https://students.cs.byu.edu/~cs345ta/labs/P6-FAT%20Supplement.html
// https://samskalicky.wordpress.com/2013/08/08/a-look-back-at-the-fat12-file-system/
// https://eric-lo.gitbook.io/lab9-filesystem/overview-of-fat32
// https://wiki.osdev.org/FAT
// http://elm-chan.org/docs/fat_e.html
// https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system
// SCSI
// https://aidanmocke.com/blog/2020/12/30/USB-MSD-1/
