#ifndef MBC5_H_
#define MBC5_H_

#include <stdint.h>

#define ENABLE_RAM_ACCESS_ADDR  0x1000
#define ENABLE_RAM_ACCESS_DATA  0xA

#define LOW_ROM_BANK_ADDR       0x2000
#define HIGH_ROM_BANK_ADDR      0x3000
#define RAM_BANK_ADDR           0x4000
#define RUMBLE_BIT              0b100

// Public functions
void mbc5_rom_dump(uint8_t *buf, uint16_t start_bank, uint16_t end_bank);
void mbc5_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank);

// Private
void mbc5_set_rom_bank(uint16_t bank);
void mbc5_set_ram_bank(uint8_t bank);
void mbc5_set_ram_access(uint8_t on_off);

#endif