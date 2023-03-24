#include "bsp/board.h"
#include "tusb.h"
#include "class/msc/msc.h"
#include "Fat16Struct.h"
#include "pico.h"
#include <hardware/flash.h>
#include "msc_disk.h"
#include "gb_disk.h"
#include "mappers/gbcam.h"


void software_reset()
{
    // watchdog_enable(1, 1); // comment out so it stops bothering me, don't know where this is
    while(1);
}

// struct flashingLocation {
//   unsigned int pageCountFlash;
// }flashingLocation = {.pageCountFlash = 0};

/*  - TinyUSB Function Callbacks -  */

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
    addr = DISK_fatTable + ((lba - file_lba_indexes[FILE_INDEX_FAT_TABLE_1_START]) * BLOCK_SIZE) + offset;
  }
  else if((lba >= file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START]) && (lba < (file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START] + FAT_TABLE_BLOCK_SIZE)))
  {
    addr = DISK_fatTable + ((lba - file_lba_indexes[FILE_INDEX_FAT_TABLE_2_START]) * BLOCK_SIZE) + offset;
  }
  else if((lba >= file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY]) && (lba < file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY] + BLOCK_SIZE_ROOT_DIRECTORY))
  {
    addr = DISK_rootDirectory + ((lba - file_lba_indexes[FILE_INDEX_ROOT_DIRECTORY]) * BLOCK_SIZE) + offset;
  }
  else if(lba >= file_lba_indexes[FILE_INDEX_STATUS_FILE] && lba < file_lba_indexes[FILE_INDEX_ROM_BIN])
  {
    addr = DISK_status_file; // TODO: This will fail if/when the status file is bigger than one block
  }
  else if(lba >= file_lba_indexes[FILE_INDEX_ROM_BIN] && lba <  file_lba_indexes[FILE_INDEX_SRAM_BIN]){
    (*the_cart.rom_memcpy_func)(buffer, ((lba - file_lba_indexes[FILE_INDEX_ROM_BIN]) * BLOCK_SIZE) + offset, bufsize);
    return (int32_t) bufsize;
  }
  else if(lba >= file_lba_indexes[FILE_INDEX_SRAM_BIN] && lba < file_lba_indexes[FILE_INDEX_PHOTOS_START] ){
    (*the_cart.ram_memcpy_func)(buffer, ((lba - file_lba_indexes[FILE_INDEX_SRAM_BIN]) * BLOCK_SIZE) + offset, bufsize);
    // memset(buffer, 0, bufsize); // TODO
    return (int32_t) bufsize;
  }
  // TODO: Not entering here when opening a photo
  else if((lba >= file_lba_indexes[FILE_INDEX_PHOTOS_START] ) && (lba < file_lba_indexes[FILE_INDEX_PHOTOS_END])){
    // Pull the right photo to working memory
    // Determine the photo being asked for by the lba
    gbcam_pull_photo(LBA2PHOTO(lba - file_lba_indexes[FILE_INDEX_PHOTOS_START]));
    // Copy the correct block of photo from working memory to the buffer
    memcpy(buffer, (working_mem + LBA2PHOTOOFFSET(lba - file_lba_indexes[FILE_INDEX_PHOTOS_START] ) * BLOCK_SIZE)  + offset, bufsize);
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
    //memcpy(&flashingLocation.buff[flashingLocatio.sectionCount * 512], buffer, bufsize);
    //uint32_t ints = save_and_disable_interrupts();
    (*the_cart.ram_memset_func)(buffer, ((lba - file_lba_indexes[FILE_INDEX_DATA_END]) * BLOCK_SIZE) + offset, bufsize);
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