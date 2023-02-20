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
  // Assuming block and cluster size stay standard (512 and 8 respectfully) this is 4k
  DISK_CLUSTER_BYTES = DISK_BLOCK_SIZE * DISK_CLUSTER_SIZE
};

enum 
{
  INDEX_RESERVED = 0,
  INDEX_FAT_TABLE_1_START = 1, //Fat table size is 0x81
  INDEX_FAT_TABLE_2_START = 0x82,
  INDEX_ROOT_DIRECTORY = 0x103,
  INDEX_DATA_STARTS = 0x123,
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
    0xEB, 0x3C, 0x90, 
    0x4D, 0x53, 0x57, 0x49, 0x4E, 0x34, 0x2E, 0x31, //MSWIN4.1
    0x00, 0x02, //sector 512
    0x08,       //cluster size 8 sectors -unit for file sizes
    0x01, 0x00, //BPB_RsvdSecCnt
    0x02,       //Number of fat tables
    0x00, 0x02, //BPB_RootEntCnt  
    0x00, 0x00, //16 bit fat sector count - 0 larger then 0x10000
    0xF8,       //- non-removable disks a 
    0x81, 0x00, //BPB_FATSz16 - Size of fat table
    0x01, 0x00, //BPB_SecPerTrk 
    0x01, 0x00, //BPB_NumHeads
    0x01, 0x00, 0x00, 0x00, //??? BPB_HiddSec 
    0xFF, 0xFF, 0x03, 0x00, //BPB_TotSec32
    0x00,  //BS_DrvNum  - probably be 0x80 but is not? 
    0x00,  //
    0x29,
    0x50, 0x04, 0x0B, 0x00, //Volume Serial Number
    'G' , 'B' , 'P' , 'U' , 'N' , 'K' , ' ' , ' ' , ' ' , ' ' , ' ' , 
    'F', 'A', 'T', '1', '6', ' ', ' ', ' ', 

    0x00, 0x00,

    // Zero up to 2 last bytes of FAT magic code
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
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
};

uint8_t DISK_fatTable[DISK_BLOCK_SIZE] =
{
    0xFF, 0xFF, 
    0xFF, 0xFF, 
    0x03, 0x00, // at least 32k rom file
    0x04, 0x00,
    0x05, 0x00,
    0x06, 0x00,
    0x07, 0x00,
    0x08, 0x00,
    0x09, 0x00,
    0xFF, 0xFF,
    0xFF, 0xFF,
};

uint8_t DISK_rootDirectory[DISK_BLOCK_SIZE] = 
{
      // first entry is volume label
      ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , ' ' , 0x28, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

      ' ' , ' ' , ' ' , ' ' , '_' , 'r' , 'o' , 'm' , 'b' , 'i' , 'n' , 0x21, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 
      0x02, 0x00, //cluster location (2)
      0x00, 0x4, 0x00, 0x00, // Filesize

      ' ' , ' ' , ' ' , ' ' , '_' , 'r' , 'a' , 'm' , 's' , 'a' , 'v' , 0x21, 0x00, 0xC6, 0x52, 0x6D,
      0x65, 0x43, 0x65, 0x43, 0x00, 0x00, 0x88, 0x6D, 0x65, 0x43, 
      0x0B, 0x00, //cluster location (3)
      0x00, 0x2, 0x00, 0x00 // Filesize (1024 bytes)
};

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
  else if(lba >= INDEX_DATA_STARTS && lba <= INDEX_DATA_END )
  {
    // printf("lba %d, bufsize %d, offset %d\n",lba, bufsize, offset);
    // mbc5_memcpy_rom(buffer, 0x104, 0x30);
    rom_only_memcpy_rom(buffer, (lba - INDEX_DATA_STARTS) * DISK_BLOCK_SIZE, bufsize);
    return (int32_t) bufsize;
  }
  if(addr != 0)
  {
    memcpy(buffer, addr, bufsize);
  }
  else{
    memset(buffer,0, bufsize);
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
  memcpy(DISK_rootDirectory + 32, cart->title, 4);
  // Get the number of clusters needed for RAM and ROM
  // Minimum for ROM is 32k (0x0 - 0x8000), which is 8 clusters
  uint16_t rom_clusters = cart->rom_size_bytes / DISK_CLUSTER_BYTES;
  // If there's a remainder, we need another cluster
  if(cart->rom_size_bytes % DISK_CLUSTER_BYTES){
    rom_clusters++;
  }
  // Set the filesize in bytes for ROM
  // DISK_rootDirectory[32 + 28] = cart->rom_size_bytes & 0xFF;
  // DISK_rootDirectory[32 + 29] = (cart->rom_size_bytes & 0xFF00) >> 8;
  // DISK_rootDirectory[32 + 30] = (cart->rom_size_bytes & 0xFF0000) >> 16;
  // DISK_rootDirectory[32 + 31] = (cart->rom_size_bytes & 0xFF000000) >> 24;
  for(uint8_t i = 0; i < 4; i++){
   DISK_rootDirectory[32 + 28 + i] = (cart->rom_size_bytes & (0xFF << (i * 8))) >> i * 8;
  }
  // Populate the FAT table
  memset(DISK_fatTable, 0xFF, 81); // Initialize
  // Starting at entry 2 (byte 4), populate the FAT table
  uint16_t current_cluster = 2;
  for(uint16_t i = 0; i < rom_clusters; i++){
    uint16_t next_entry = current_cluster + 1;
    DISK_fatTable[current_cluster * 2] = next_entry & 0xFF;
    DISK_fatTable[(current_cluster * 2) + 1] = (next_entry & 0xFF00) >> 8;
    current_cluster++;
  }
  // Last cluster is EOF
  DISK_fatTable[4 + (rom_clusters * 2)] = 0xFF;
  DISK_fatTable[4 + (rom_clusters * 2) + 1] = 0xFF;
  // TODO: 
  // Handle FAT tables that are not 0x81 in size
  // Handle FAT tables that are larger than a single block 
  // Handle FAT tables that are larger than a cluster

  // HANDLE RAM INFO
  // Set RAM name if RAM exists, otherwise zero it out
  if(cart->ram_size_bytes){
    memcpy(DISK_rootDirectory + 64, cart->title, 4);
  }
  else{
    memset(DISK_rootDirectory + 64, 0, 32);
  }
}