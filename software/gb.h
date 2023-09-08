#ifndef GB_H_
#define GB_H_
// Low level workings of the gameboy and
#include <stdint.h>

#define ROM_BANK0_START_ADDR	0x0
#define ROM_BANK0_END_ADDR		0x3FFF
#define ROM_BANK_SIZE         (ROM_BANK0_END_ADDR + 1)
#define ROM_BANKN_START_ADDR	0x4000
#define ROM_BANKN_END_ADDR		0x7FFF
#define SRAM_START_ADDR     	0xA000
#define SRAM_END_ADDR       	0xBFFF
#define SRAM_BANK_SIZE        (SRAM_END_ADDR - SRAM_START_ADDR + 1)
#define SRAM_HALF_BANK_SIZE   (SRAM_BANK_SIZE / 2)
extern uint8_t working_mem[ROM_BANK_SIZE * 2];

// TODO: inline or #define these. They should go somewhere else
// Get the ROM bank from the perspective of the FAT16 filesystem
// Ex: addr 0x0 = ROM bank 0. addr 0x4112 = ROM bank 1. Addr 0x8334 = ROM bank 2
uint16_t fs_get_rom_bank(uint32_t addr);
// Get the RAM bank from the perspective of the FAT16 filesystem
// RAM is its own file, so it starts at 0x0
// Ex: addr 0x0 = RAM bank 0. addr 0x2882 = RAM bank 1
uint16_t fs_get_ram_bank(uint32_t addr);


uint8_t readb(uint16_t addr);
void writeb(uint8_t data, uint16_t addr);
void readbuf(uint16_t addr, uint8_t *buf, uint16_t len);
void set_dbus_direction(uint8_t dir);
void init_bus();
void reset_pin_states();
void reset_game();
void pulse_clock();

#endif