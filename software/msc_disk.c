#include "bsp/board.h"
#include "tusb.h"
#include "class/msc/msc.h"
#include "Fat16Struct.h"
#include "pico.h"
#include <hardware/flash.h>
#include "checkUF2.h"
#include "rom_only.h"
#include "msc_disk.h"


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

// whether host does safe-eject
static bool ejected = false;

// Some MCU doesn't have enough 8KB SRAM to store the whole disk
// We will use Flash as read-only disk with board that has

void software_reset()
{
    watchdog_enable(1, 1);
    while(1);
}

enum
{
  DISK_BLOCK_NUM  = 0x200000, // Pretend to be a 1GB drive
  DISK_BLOCK_SIZE = 512,
  DISK_CLUSTER_SIZE = 8,
  DISK_CLUSTER_BYTES = DISK_BLOCK_SIZE * DISK_CLUSTER_SIZE, // Assuming block and cluster size stay standard (512 and 8 respectfully) this is 4k
  // Hold enough sectors for every file I want
  // 1 sector for status.txt
  // 8192 sectors for 32MB ROM
  // 8192 sectors for 32MB SRAM
  // 2 sectors (8K) for each photo, 32 photos total
  // Two bytes per entry (FAT16 = 16 bit entries)
  FAT_TABLE_SIZE = (1 + 8192 + 8192 + (2 * 32)) * 2, 
  // Root directory should be large enough to hold every file needed
  // 1 status file
  // 1 ROM file
  // 1 SRAM file
  // 32 Photo files
  // 32 bytes per entry
  ROOT_DIRECTORY_SIZE = (1 + 1 + 1 + 32) * 32, 
  STATUS_FILE_SIZE = DISK_BLOCK_SIZE, // Small for now, can be up to 1 cluster (4k) with current layout
};

enum 
{
  INDEX_RESERVED          = 0x00000,  // First cluster is for all the FAT magic data
  INDEX_FAT_TABLE_1_START = 0x00001,  // FAT table starts at 0x1, 0x81 blocks in size
  INDEX_FAT_TABLE_2_START = 0x00082,  // Redundant FAT table is 0x81 blocks later
  INDEX_ROOT_DIRECTORY    = 0x00103,  // Root directory starts after second FAT table
  INDEX_DATA_STARTS       = 0x00123,  // Root directory is 0x20 blocks (32 * 512 = 16384 bytes). Can store 16384 / 32 = 512 files total
  INDEX_STATUS_FILE       = 0x00123,  // Status file is the first one in the data section, 
  INDEX_ROM_BIN           = 0x0012B,  // ROM bin starts after status file (status file 1 cluster large, 8 blocks)
  INDEX_SRAM_BIN          = 0x1012B,  // SRAM bin starts after ROM bin (ROM bin 8192 clusters large, 65536 blocks, 32 MB total filesize)
  INDEX_PHOTOS_START      = 0x2012B,  // Photos starts after SRAM bin (SRAM bin 8192 clusters large, 65536 blocks, 32 MB total filesize)
  INDEX_DATA_END = DISK_BLOCK_NUM
};

#define MAX_SECTION_COUNT_FOR_FLASH_SECTION (FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE)
struct flashingLocation {
  unsigned int pageCountFlash;
}flashingLocation = {.pageCountFlash = 0};

// TODO:
// Generate a FAT filesystem on the fly
// will need to handle:
// - Generating reserved section. Change name of drive to name of game
// - Generating FAT table. Will need to handle arbitrary amounts of files and file sizes
// - Generating root directory. Will need to handle arbitrary amounts of files and file sizes
// - Translation layer for FS to GB. 

uint8_t DISK_reservedSection[DISK_BLOCK_SIZE] = 
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

uint8_t DISK_fatTable[FAT_TABLE_SIZE] ={0};

uint8_t DISK_rootDirectory[ROOT_DIRECTORY_SIZE] = 
{
      // first entry is volume label
      ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , 0x28, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      's' , 't' , 'a' , 't' , 'u' , 's' , ' ' , ' ' , 't' , 'x' , 't' , 0x21, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 
      0x02, 0x00, //cluster location (2)
      0x00, 0x02, 0x00, 0x00, // Filesize in bytes (512)

      ' ' , ' ' , ' ' , ' ' , '_' , 'r' , 'o' , 'm' , 'b' , 'i' , 'n' , 0x21, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 
      0x03, 0x00, //cluster location (3)
      0x00, 0x00, 0x00, 0x00, // Filesize in bytes (unititialized)

      ' ' , ' ' , ' ' , ' ' , '_' , 'r' , 'a' , 'm' , 's' , 'a' , 'v' , 0x21, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 
      0x03, 0x20, //cluster location (8195)
      0x00, 0x00, 0x00, 0x00 // Filesize area in bytes (unititialized)
};

uint16_t status_file_ptr = 0;
uint8_t DISK_status_file[STATUS_FILE_SIZE] = {0};
void append_status_file(const uint8_t* buf){
  uint16_t buf_index = 0;
  for(;;){
    if(status_file_ptr > STATUS_FILE_SIZE - 1){
      return;
    }
    if(buf[buf_index] == '\0'){
      return;
    }
    DISK_status_file[status_file_ptr] = buf[buf_index];
    status_file_ptr++;
    buf_index++;
  }
}

void init_disk_mem(){
  memset(DISK_status_file, ' ', STATUS_FILE_SIZE);
  memset(DISK_fatTable, 0xFF, FAT_TABLE_SIZE);
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

  *block_count = DISK_BLOCK_NUM;
  *block_size  = DISK_BLOCK_SIZE;
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
  if ( lba >= DISK_BLOCK_NUM ) return -1;
  // printf("lba 0x%x, bufsize %d, offset %d\n",lba, bufsize, offset);
  uint8_t const* addr = 0;
  // memcpy(buffer, addr, bufsize);

  // uint8_t * addr = 0;
  if(lba == INDEX_RESERVED)
  {
    addr = DISK_reservedSection;
  }
  else if(lba == INDEX_FAT_TABLE_1_START || lba == INDEX_FAT_TABLE_2_START)
  {
    addr = DISK_fatTable;
  }
  else if(lba == INDEX_ROOT_DIRECTORY)
  {
    addr = DISK_rootDirectory;
  }
  else if(lba >= INDEX_STATUS_FILE && lba < INDEX_ROM_BIN)
  {
    addr = DISK_status_file;
  }
  else if(lba >= INDEX_ROM_BIN && lba <  INDEX_SRAM_BIN){
    mbc5_memcpy_rom(buffer, (lba - INDEX_ROM_BIN) * DISK_BLOCK_SIZE, bufsize);
    // rom_only_memcpy_rom(buffer, (lba - INDEX_ROM_BIN) * DISK_BLOCK_SIZE, bufsize);
    return (int32_t) bufsize;
  }
  else if(lba >= INDEX_SRAM_BIN && lba <  INDEX_PHOTOS_START){
    // mbc5_memcpy_ram(buffer, (lba - INDEX_ROM_BIN) * DISK_BLOCK_SIZE, bufsize);
    memset(buffer, 0, bufsize); // TODO
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

bool tud_msc_is_writable_cb (uint8_t lun)
{
  (void) lun;


  return true;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  (void) lun;
  printf("write - lba 0x%x, bufsize%d\n", lba,bufsize);
  
  // out of ramdisk
  if ( lba >= DISK_BLOCK_NUM ) return -1;
  //page to sector
  if(lba >= INDEX_DATA_END){
    //memcpy(&flashingLocation.buff[flashingLocation.sectionCount * 512], buffer, bufsize);
    //uint32_t ints = save_and_disable_interrupts();
    if(flashingLocation.pageCountFlash % MAX_SECTION_COUNT_FOR_FLASH_SECTION == 0)
    {
      printf("doing flash at offset 0x%x\n",flashingLocation.pageCountFlash * FLASH_PAGE_SIZE);
      flash_range_erase((flashingLocation.pageCountFlash / 16) * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    }
    unsigned int * returnSize;
    unsigned char *addressUF2 = getUF2Info(buffer,returnSize);
    if(addressUF2 != 0)
    {
      printf("UF2 FILE %d!!!\n",flashingLocation.pageCountFlash );
      flash_range_program(flashingLocation.pageCountFlash * FLASH_PAGE_SIZE,
                          &buffer[32], 
                          256);
      flashingLocation.pageCountFlash += 1; //(*returnSize/ FLASH_PAGE_SIZE);
    }
    else{
      flash_range_program(flashingLocation.pageCountFlash * FLASH_PAGE_SIZE,
                          buffer, 
                          bufsize);
      flashingLocation.pageCountFlash += (bufsize/ FLASH_PAGE_SIZE);
    }
  }

  if(lba == INDEX_ROOT_DIRECTORY)
  {
    printf("\n\n\nRestarting the raspberry pi pico!!!! \n\n\n");
    sleep_ms(100);//just to make sure all uart get out

    software_reset();
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


void init_disk(struct Cart* cart){
  // HANDLE VOLUME INFO
  // First, initialize the names for everything
  // Set the volume label to the cart name (first 8 chars)
  memcpy(DISK_rootDirectory, cart->title, 8);

  // HANDLE ROM INFO
  // Set the ROM names
  memcpy(DISK_rootDirectory + 64, cart->title, 4);
  // Get the number of clusters needed for RAM and ROM
  // Minimum for ROM is 32k (0x0 - 0x8000), which is 8 clusters
  uint16_t rom_clusters = cart->rom_size_bytes / DISK_CLUSTER_BYTES;
  // If there's a remainder, we need another cluster
  if(cart->rom_size_bytes % DISK_CLUSTER_BYTES){
    rom_clusters++;
  }
  // Set the filesize in bytes for ROM
  // DISK_rootDirectory[64 + 28] = cart->rom_size_bytes & 0xFF;
  // DISK_rootDirectory[64 + 29] = (cart->rom_size_bytes & 0xFF00) >> 8;
  // DISK_rootDirectory[64 + 30] = (cart->rom_size_bytes & 0xFF0000) >> 16;
  // DISK_rootDirectory[64 + 31] = (cart->rom_size_bytes & 0xFF000000) >> 24;
  for(uint8_t i = 0; i < 4; i++){
   DISK_rootDirectory[64 + 28 + i] = (cart->rom_size_bytes & (0xFF << (i * 8))) >> i * 8;
  }
  // Populate the FAT table
  // TODO: This is generating invalid cluster chains for pokemon yellow. Not sure why
  // Starting at entry 3 (byte 6), populate the FAT table with ROM data
  uint16_t current_cluster = 3;
  for(uint16_t i = 0; i < rom_clusters; i++){
    uint16_t next_entry = current_cluster + 1;
    DISK_fatTable[current_cluster * 2] = next_entry & 0xFF;
    DISK_fatTable[(current_cluster * 2) + 1] = (next_entry & 0xFF00) >> 8;
    current_cluster++;
  }
  // Last cluster is EOF
  DISK_fatTable[(current_cluster * 2)] = 0xFF;
  DISK_fatTable[(current_cluster * 2) + 1] = 0xFF;

  // HANDLE RAM INFO
  // Set RAM name if RAM exists, otherwise zero it out
  memset(DISK_rootDirectory + 96, 0, 32);
  // if(cart->ram_size_bytes){
  //   memcpy(DISK_rootDirectory + 96, cart->title, 4);
  // }
  // else{
  //   memset(DISK_rootDirectory + 96, 0, 32);
  // }
}