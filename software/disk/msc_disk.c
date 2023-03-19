#include "bsp/board.h"
#include "tusb.h"
#include "class/msc/msc.h"
#include "Fat16Struct.h"
#include "pico.h"
#include <hardware/flash.h>
#include "cart.h"
#include "msc_disk.h"
#include "mappers/gbcam.h"
#include "gb.h"


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

/*  - Enums -  */
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

enum {
  INDEX_CLUSTER_SIZE_STATUS_FILE  = 0,
  INDEX_CLUSTER_SIZE_ROM_FILE     = 1,
  INDEX_CLUSTER_SIZE_RAM_FILE     = 2,
  INDEX_CLUSTER_SIZE_PHOTOS       = 3
};

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

enum {
  INDEX_CLUSTER_START_FS = 0,
  INDEX_CLUSTER_START_STATUS_FILE = 1,
  INDEX_CLUSTER_START_ROM_FILE = 2,
  INDEX_CLUSTER_START_RAM_FILE = 3,
  INDEX_CLUSTER_START_PHOTOS = 4
};

/*  - Private #defines -  */
#define ROOT_DIR_ENTRY(X)     X*ROOT_DIR_ENTRY_SIZE
// Convert cluster size to blocks size
#define CLS2BLK(x)  (x*CLUSTER_BLOCK_SIZE)
// Convert block size to byte size
#define BLK2BYTE(x) (x*BLOCK_SIZE)
// Convert cluster size to block size
#define CLS2BYTE(x) BLK2BYTE(CLS2BLK(x))


/*  - Private Variables -  */
// whether host does safe-eject
static bool ejected = false;
// The most recent root directory entry
uint32_t latest_rd_entry = 0;
// The most recent FAT table entry byte
uint32_t latest_fat_entry = 0;
// The file entries of all the file indexes
uint32_t file_lba_indexes[30] = {0};
// The cluster sizes of all the files
uint32_t file_cluster_sizes[4] = {0};
// The starting clusters of all the files
// TODO: This should eventually just be removed and replaced with file_lba_indexes
uint32_t file_starting_clusters[5] = {0};
// The size of the status file
uint16_t status_file_size = 0;
// A blank root directory entry to use
uint8_t blank_rd_entry[] = {      
  ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , // filename (uninitialized)
  0x21, 0x00, 0xC6, 0x52, 0x6D, 0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, // Bunch of unknown options
  0x00, 0x00, //cluster location (unititialized)
  0x00, 0x00, 0x00, 0x00, // Filesize in bytes (unititialized)}
};
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

// Some MCU doesn't have enough 8KB SRAM to store the whole disk
// We will use Flash as read-only disk with board that has
void rd_set_file_size(uint32_t entry, uint32_t filesize);

void software_reset()
{
    // watchdog_enable(1, 1); // comment out so it stops bothering me, don't know where this is
    while(1);
}

void set_file_cluster_sizes(){
  file_cluster_sizes[INDEX_CLUSTER_SIZE_STATUS_FILE] = 1;
  file_cluster_sizes[INDEX_CLUSTER_SIZE_ROM_FILE] = 8192;
  file_cluster_sizes[INDEX_CLUSTER_SIZE_RAM_FILE] = 8192;
  file_cluster_sizes[INDEX_CLUSTER_SIZE_PHOTOS] = 2;
}

void set_file_lba_indexes(){
  file_lba_indexes[FILE_INDEX_RESERVED]               = 0x00000;
  file_lba_indexes[FILE_INDEX_FAT_TABLE_1_START]      = 0x00001;
  file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START]      = 0x00082;
  file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY]         = 0x00103;
  file_lba_indexes[FILE_INDEX_DATA_STARTS]            = 0x00123;
  file_lba_indexes[FILE_INDEX_STATUS_FILE]            = file_lba_indexes[FILE_INDEX_DATA_STARTS];
  file_lba_indexes[FILE_INDEX_ROM_BIN]                = file_lba_indexes[FILE_INDEX_STATUS_FILE] + CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_STATUS_FILE]);
  file_lba_indexes[FILE_INDEX_SRAM_BIN]               = file_lba_indexes[FILE_INDEX_ROM_BIN] + CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_ROM_FILE]);
  file_lba_indexes[FILE_INDEX_PHOTOS_START]           = file_lba_indexes[FILE_INDEX_SRAM_BIN] + CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_RAM_FILE]);
  file_lba_indexes[FILE_INDEX_PHOTOS_END]             = file_lba_indexes[FILE_INDEX_PHOTOS_START] + (CLS2BLK(file_cluster_sizes[INDEX_CLUSTER_SIZE_PHOTOS]) * 30);
  file_lba_indexes[FILE_INDEX_DATA_END]               = file_lba_indexes[FILE_INDEX_PHOTOS_END];
}

void set_starting_clusters(){
  file_starting_clusters[INDEX_CLUSTER_START_FS] = 2;
  file_starting_clusters[INDEX_CLUSTER_START_STATUS_FILE] = file_starting_clusters[INDEX_CLUSTER_START_FS];
  file_starting_clusters[INDEX_CLUSTER_START_ROM_FILE] = file_starting_clusters[INDEX_CLUSTER_START_STATUS_FILE] + file_cluster_sizes[INDEX_CLUSTER_SIZE_STATUS_FILE];
  file_starting_clusters[INDEX_CLUSTER_START_RAM_FILE] = file_starting_clusters[INDEX_CLUSTER_START_ROM_FILE] + file_cluster_sizes[INDEX_CLUSTER_SIZE_ROM_FILE];
  file_starting_clusters[INDEX_CLUSTER_START_PHOTOS] = file_starting_clusters[INDEX_CLUSTER_START_RAM_FILE] + file_cluster_sizes[INDEX_CLUSTER_SIZE_RAM_FILE];
}


// struct flashingLocation {
//   unsigned int pageCountFlash;
// }flashingLocation = {.pageCountFlash = 0};

// TODO:
// Generate a FAT filesystem on the fly
// will need to handle:
// - Generating reserved section. Change name of drive to name of game
// - Generating FAT table. Will need to handle arbitrary amounts of files and file sizes
// - Generating root directory. Will need to handle arbitrary amounts of files and file sizes
// - Translation layer for FS to GB. 





uint8_t DISK_status_file[STATUS_FILE_SIZE] = {0};
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

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  (void) lun;

  const char vid[] = "kidoutai";
  const char pid[] = "GBPunk";
  const char rev[] = "1.0";

  memcpy(vendor_id  , vid, strlen(vid));
  memcpy(product_id , pid, strlen(pid));
  memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  (void) lun;

  // RAM disk is ready until ejected
  if (ejected) {
    // Additional Sense 3A-00 is NOT_FOUND
    tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
    return false;
  }

  return true;
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  (void) lun;

  *block_count = DISK_BLOCK_COUNT;
  *block_size  = BLOCK_SIZE;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  (void) lun;
  (void) power_condition;

  if ( load_eject )
  {
    if (start)
    {
      // load disk storage
    }else
    {
      // unload disk storage
      ejected = true;
    }
  }

  return true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  (void) lun;

  // out of ramdisk
  if ( lba >= DISK_BLOCK_COUNT ) return -1;
  // printf("lba 0x%x, bufsize %d, offset %d\n",lba, bufsize, offset);
  uint8_t const* addr = 0;
  // memcpy(buffer, addr, bufsize);

  // uint8_t * addr = 0;
  if(lba == file_lba_indexes[FILE_INDEX_RESERVED])
  {
    addr = DISK_reservedSection;
  }

  //Need to handle both FAT tables seperately (TODO: Just tell the thing there's only one table)
  else if((lba >= file_lba_indexes[FILE_INDEX_RESERVED]) && (lba < (file_lba_indexes[FILE_INDEX_FAT_TABLE_1_START] + FAT_TABLE_BLOCK_SIZE)))
  {
    addr = DISK_fatTable + ((lba - file_lba_indexes[FILE_INDEX_FAT_TABLE_1_START]) * BLOCK_SIZE);
  }
  else if((lba >= file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START]) && (lba < (file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START] + FAT_TABLE_BLOCK_SIZE)))
  {
    addr = DISK_fatTable + ((lba - file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START]) * BLOCK_SIZE);
  }
  else if((lba >= file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY]) && (lba < file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY] + BLOCK_SIZE_ROOT_DIRECTORY))
  {
    addr = DISK_rootDirectory + ((lba - file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY]) * BLOCK_SIZE);
  }
  else if(lba >= file_lba_indexes[FILE_INDEX_STATUS_FILE] && lba < file_lba_indexes[FILE_INDEX_ROM_BIN])
  {
    addr = DISK_status_file; // TODO: This will fail if/when the status file is bigger than one block
  }
  else if(lba >= file_lba_indexes[FILE_INDEX_ROM_BIN] && lba <  file_lba_indexes[FILE_INDEX_ROM_BIN]){
    (*the_cart.rom_memcpy_func)(buffer, ((lba - file_lba_indexes[FILE_INDEX_ROM_BIN]) * BLOCK_SIZE) + offset, bufsize);
    return (int32_t) bufsize;
  }
  else if(lba >= file_lba_indexes[FILE_INDEX_SRAM_BIN] && lba <  file_lba_indexes[FILE_INDEX_PHOTOS_START] ){
    (*the_cart.ram_memcpy_func)(buffer, ((lba - file_lba_indexes[FILE_INDEX_SRAM_BIN]) * BLOCK_SIZE) + offset, bufsize);
    // memset(buffer, 0, bufsize); // TODO
    return (int32_t) bufsize;
  }
  // TODO: Will probably need to deal with offset here
  else if((lba >= file_lba_indexes[FILE_INDEX_PHOTOS_START] ) && (lba < file_lba_indexes[FILE_INDEX_PHOTOS_END])){
    // Pull the right photo to working memory
    // Determine the photo being asked for by the lba
    gbcam_pull_photo(LBA2PHOTO(lba-file_lba_indexes[FILE_INDEX_PHOTOS_START] ));
    // Copy the correct block of photo from working memory to the buffer
    memcpy(buffer, working_mem + LBA2PHOTOOFFSET(lba-file_lba_indexes[FILE_INDEX_PHOTOS_START] ) * BLOCK_SIZE, bufsize);
    return (int32_t) bufsize;
  }
  if(addr != 0)
  {
    memcpy(buffer, addr, bufsize);
  }
  else{
    memset(buffer, 0, bufsize);
  }
  return (int32_t) bufsize;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  (void) lun;
  // printf("write - lba 0x%x, bufsize%d\n", lba,bufsize);
  
  // out of ramdisk
  if ( lba >= DISK_BLOCK_COUNT ) return -1;
  //page to sector
  if(lba >= file_lba_indexes[FILE_INDEX_DATA_END]){
    //memcpy(&flashingLocation.buff[flashingLocation.sectionCount * 512], buffer, bufsize);
    //uint32_t ints = save_and_disable_interrupts();
    printf("0x%x\n", lba);
  }

  if(lba == file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY])
  {
    printf("0x%x\n", lba);
  }

//#ifndef CFG_EXAMPLE_MSC_READONLY
//  uint8_t* addr = msc_disk[lba] + offset;
//  memcpy(addr, buffer, bufsize);
//#else
  

  return (int32_t) bufsize;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks
int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
  // read10 & write10 has their own callback and MUST not be handled here

  void const* response = NULL;
  int32_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, (size_t) resplen);
    }else
    {
      // SCSI output
    }
  }

  return (int32_t) resplen;
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