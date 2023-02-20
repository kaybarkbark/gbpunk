#include "rom_only.h"

uint16_t rom_only_memcpy_rom(uint8_t* dest, uint16_t rom_addr, uint16_t num){
    for(uint16_t i = 0; i < num; i++){
        dest[i] = readb(i + rom_addr);
    }
}

