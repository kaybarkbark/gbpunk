#ifndef HUC1_H_
#define HUC1_H_

#include <stdint.h>
#include "gb.h"


#define HUC1_IR_SRAM_SELECT_ADDR    0x1000
// Note: Writing a 0x0 seems to disable all writes to SRAM, but allows reads. Look in to further, could be undocumented feature
#define HUC1_IR_SRAM_SELECT_IR      0x0E
#define HUC1_IR_SRAM_SELECT_SRAM    0x0A

#define HUC1_RAM_BANK_ADDR      0x5000
#define HUC1_ROM_BANK_ADDR      0x3000

void huc1_memcpy_rom(uint8_t* dest, uint32_t rom_addr, uint32_t num);
void huc1_memcpy_ram(uint8_t* dest, uint32_t ram_addr, uint32_t num);
void huc1_memset_ram(uint8_t* buf, uint32_t ram_addr, uint32_t num);
void huc1_set_rom_bank(uint16_t bank);
void huc1_set_ram_bank(uint16_t bank);
void huc1_set_ram_access(uint8_t on_off);

#endif