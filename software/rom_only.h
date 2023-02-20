#ifndef ROM_ONLY_H_
#define ROM_ONLY_H_

#include <stdint.h>
#include "gb.h"

// Public functions
uint16_t rom_only_memcpy_rom(uint8_t* dest, uint16_t rom_addr, uint16_t num);

#endif