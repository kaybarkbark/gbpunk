#ifndef MBC5_H_
#define MBC5_H_

#include <stdint.h>
#include "gb.h"

#define MBC5_ENABLE_RAM_ACCESS_ADDR  0x1000
#define MBC5_ENABLE_RAM_ACCESS_DATA  0xA

#define MBC5_LOW_ROM_BANK_ADDR       0x2000
#define MBC5_HIGH_ROM_BANK_ADDR      0x3000
#define MBC5_RAM_BANK_ADDR           0x4000
#define MBC5_RUMBLE_BIT              0b100

// Public functions
void mbc5_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
// Note: This gets memory relative to RAM, not the cart. So 0x0 means start of RAM
void mbc5_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);
void mbc5_rom_dump(uint8_t *buf, uint16_t start_bank, uint16_t end_bank);
void mbc5_ram_dump(uint8_t *buf, uint8_t start_bank, uint8_t end_bank);

// Private
void mbc5_set_rom_bank(uint16_t bank);
void mbc5_set_ram_bank(uint16_t bank);
void mbc5_set_ram_access(uint8_t on_off);

#endif