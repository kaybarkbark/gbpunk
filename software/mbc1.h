#ifndef MBC1_H_
#define MBC1_H_

#include <stdint.h>
#include "gb.h"

#define MBC1_ENABLE_RAM_ACCESS_ADDR     0x1000
#define MBC1_ENABLE_RAM_ACCESS_DATA     0xA

#define MBC1_ROM_BANK_LOWER_BITS        0x2000
#define MBC1_RAM_ROM_SHARED_BANK        0x4000
#define MBC1_ROM_RAM_SELECT             0x6000

// Public functions
void mbc1_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
void mbc1_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);


// Private
void mbc1_set_rom_bank(uint16_t bank);
void mbc1_set_ram_bank(uint16_t bank);
void mbc1_set_ram_access(uint8_t on_off);
uint8_t mbc1_check_invalid_bank(uint16_t bank);

#endif