#ifndef MBC3_H_
#define MBC3_H_

#include <stdint.h>
#include "gb.h"

#define MBC3_ENABLE_RAM_ACCESS_ADDR     0x0000
#define MBC3_ENABLE_RAM_ACCESS_DATA     0xA

#define MBC3_ROM_BANK_ADDR              0x2000
#define MBC3_RAM_BANK_ADDR              0x4000

void mbc3_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
void mbc3_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);
void mbc3_set_rom_bank(uint16_t bank);
void mbc3_set_ram_bank(uint16_t bank);
void mbc3_set_ram_access(uint8_t on_off);

#endif