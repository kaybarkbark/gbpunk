#ifndef ROM_ONLY_H_
#define ROM_ONLY_H_

#include <stdint.h>
#include "gb.h"

// Public functions
void no_mapper_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
void no_mapper_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);
void no_mapper_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num);

#endif