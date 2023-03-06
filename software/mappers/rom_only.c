#include "rom_only.h"

void rom_only_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    for(uint16_t i = 0; i < num; i++){
        dest[i] = readb(i + rom_addr);
    }
}

