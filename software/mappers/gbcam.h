#ifndef GBCAM_H_
#define GBCAM_H_

#include <stdint.h>

#define GBCAM_ENABLE_RAM_WRITE_ADDR     0x1000
#define GBCAM_ENABLE_RAM_WRITE_DATA     0xA
#define GBCAM_ROM_BANK_ADDR             0x2000
#define GBCAM_RAM_BANK_ADDR             0x4000
#define GBCAM_CAM_REG_SIZE              0x80
#define GBCAM_CAM_REG_BANK              0x10
#define GBCAM_NUM_ROM_BANKS             0x40
#define GBCAM_NUM_SRAM_BANKS            0x10

#define GBCAM_IMAGE_START_ADDR          0xD0E  // this do be lookin srs d0e
#define GBCAM_BMP_PHOTO_SIZE            7286

#define LBA2PHOTO(x) ((x)/16)
#define LBA2PHOTOOFFSET(x) ((x)%16)

extern uint8_t bmp_header[0x76];

// Public functions
void gbcam_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
void gbcam_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);
void gbcam_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num);
void gbcam_set_rom_bank(uint16_t bank);
void gbcam_set_ram_bank(uint16_t bank);
void gbcam_set_ram_access(uint8_t on_off);
void gbcam_pull_photo(uint8_t num);

// DEPRECATED
void gbcam_rom_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank);
void gbcam_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank);
uint8_t gbcam_unit_test();
uint8_t gbcam_unit_test_rom_bank_switching();
uint8_t gbcam_unit_test_sram_bank_switching();
uint8_t gbcam_unit_test_sram();

#endif