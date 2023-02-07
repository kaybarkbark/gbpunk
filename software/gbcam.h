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
// NOTE: Check this https://github.com/AntonioND/gbcam-rev-engineer/blob/master/gbcam_arduino_server/gbcam_arduino_server.ino

// Public functions
void gbcam_rom_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank);
void gbcam_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank);
// TODO: CAM register dump

// Unit Tests

uint8_t gbcam_unit_test();
uint8_t gbcam_unit_test_rom_bank_switching();
uint8_t gbcam_unit_test_sram_bank_switching();
uint8_t gbcam_unit_test_sram();

// Private
void gbcam_set_rom_bank(uint8_t bank);
void gbcam_set_ram_bank(uint8_t bank);
void gbcam_set_ram_writable(uint8_t on_off);

#endif