#ifndef MBC2_H_
#define MBC2_H_

#include <stdint.h>
#include "gb.h"

#define MBC2_ENABLE_RAM_ACCESS_ADDR     0x0000
#define MBC2_ENABLE_RAM_ACCESS_DATA     0x0
#define MBC2_DISABLE_RAM_ACCESS_DATA    0x00FF

#define MBC2_ROM_BANK_ADDR              0x2100

void mbc2_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
void mbc2_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);
void mbc2_set_rom_bank(uint16_t bank);
void mbc2_set_ram_access(uint8_t on_off);

#endif