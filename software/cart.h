#ifndef CART_H_
#define CART_H_

#include <stdint.h>

const uint8_t logo[0x30];

#define CART_TYPE_ADDR        0x147 // bank 0
#define ROM_BANK_SHIFT_ADDR 	0x148 // bank 0
#define RAM_BANK_COUNT_ADDR 	0x149 // bank 0
#define CART_TITLE_ADDR     	0x134 // bank 0
#define CART_TITLE_LEN      	16
#define LOGO_START_ADDR     	0x104
#define LOGO_END_ADDR       	0x133
#define LOGO_LEN            	(LOGO_END_ADDR - LOGO_START_ADDR + 1)

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
   void (*rom_memcpy_func)(uint8_t*, uint32_t, uint32_t); // Pointer to ROM memcpy function
   void (*ram_memcpy_func)(uint8_t*, uint32_t, uint32_t); // Pointer to RAM memcpy function
   void (*ram_memset_func)(uint8_t*, uint32_t, uint32_t); // Pointer to RAM memset function
   char title[17];
   char cart_type_str[30];
}; 

struct Cart the_cart;

uint16_t cart_check(uint8_t *logo_buf);
void populate_cart_info();
void dump_cart_info();

#endif