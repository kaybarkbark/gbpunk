#include "no_mapper.h"

void no_mapper_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num){
    for(uint16_t i = 0; i < num; i++){
        dest[i] = readb(i + rom_addr);
    }
}

void no_mapper_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num){
    for(uint32_t i = 0; i < num; i++){
        dest[i] = readb(i + SRAM_START_ADDR); 
    }
}

void no_mapper_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num){
    for(uint32_t i = 0; i < num; i++){
        writeb(buf[i], i + SRAM_START_ADDR);
    }
}