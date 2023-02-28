#ifndef ROM_ONLY_H_
#define ROM_ONLY_H_

#include <stdint.h>
#include "gb.h"

// Public functions
void rom_only_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);

#endif