#ifndef GB_H_
#define GB_H_

#include <stdint.h>
const uint8_t logo[0x30];
uint8_t working_mem[0x8000];

#define CART_TYPE_ADDR        0x147 // bank 0
#define ROM_BANK_SHIFT_ADDR 	0x148 // bank 0
#define RAM_BANK_COUNT_ADDR 	0x149 // bank 0
#define CART_TITLE_ADDR     	0x134 // bank 0
#define CART_TITLE_LEN      	16
#define LOGO_START_ADDR     	0x104
#define LOGO_END_ADDR       	0x133
#define LOGO_LEN            	(LOGO_END_ADDR - LOGO_START_ADDR + 1)

#define ROM_BANK0_START_ADDR	0x0
#define ROM_BANK0_END_ADDR		0x3FFF
#define ROM_BANK_SIZE         (ROM_BANK0_END_ADDR + 1)
#define ROM_BANKN_START_ADDR	0x4000
#define ROM_BANKN_END_ADDR		0x7FFF
#define SRAM_START_ADDR     	0xA000
#define SRAM_END_ADDR       	0xBFFF
#define SRAM_BANK_SIZE        (SRAM_END_ADDR - SRAM_START_ADDR + 1)
#define SRAM_HALF_BANK_SIZE   (SRAM_BANK_SIZE / 2)

#define MAPPER_UNKNOWN        0x0
#define MAPPER_ROM_ONLY       0x1
#define MAPPER_ROM_RAM        0x2
#define MAPPER_MBC1           0x3
#define MAPPER_MBC2           0x4
#define MAPPER_MBC3           0x5
#define MAPPER_MMM01          0x6
#define MAPPER_MBC4           0x7
#define MAPPER_MBC5           0x8
#define MAPPER_GBCAM          0x9

struct Cart {
   uint8_t  cart_type;
   uint8_t  mapper_type;
   uint8_t  rom_banks;
   uint8_t  ram_banks;
   uint16_t ram_end_address;
   uint32_t rom_size_bytes;
   uint32_t ram_size_bytes;
   void (*rom_memcpy_func)(uint8_t*, uint16_t, uint16_t); // Pointer to ROM memcpy function
   void (*ram_memcpy_func)(uint8_t*, uint16_t, uint16_t); // Pointer to RAM memcpy function
   void (*ram_memset_func)(uint8_t*, uint16_t, uint16_t); // Pointer to RAM memset function
   char title[17];
   char cart_type_str[30];
}; 

// TODO: inline or #define these
// Get the ROM bank from the perspective of the FAT16 filesystem
// Ex: addr 0x0 = ROM bank 0. addr 0x4112 = ROM bank 1. Addr 0x8334 = ROM bank 2
uint16_t fs_get_rom_bank(uint16_t addr);
// Get the RAM bank from the perspective of the FAT16 filesystem
// RAM is its own file, so it starts at 0x0
// Ex: addr 0x0 = RAM bank 0. addr 0x2882 = RAM bank 1
uint16_t fs_get_ram_bank(uint16_t addr);


uint8_t readb(uint16_t addr);
void writeb(uint8_t data, uint16_t addr);
void readbuf(uint16_t addr, uint8_t *buf, uint16_t len);
void set_dbus_direction(uint8_t dir);
void init_bus();
void reset_pin_states();
void reset_cart();
void pulse_clock();
uint16_t cart_check(uint8_t *logo_buf);
void get_cart_info(struct Cart *cart);
void dump_cart_info(struct Cart *cart);

#endif