#ifndef CART_UNIT_TESTS_H
#define CART_UNIT_TESTS_H

#include "cart.h"
#include <stdint.h>

uint8_t unit_test_cart();
uint8_t unit_test_rom_ram_coherency(
    void (*rom_memcpy_func)(uint8_t*, uint32_t, uint32_t), 
    void (*ram_memcpy_func)(uint8_t*, uint32_t, uint32_t),
    uint32_t ram_size
);
uint8_t unit_test_rom_ram_bankswitching(
    void (*rom_bankswitch_func)(uint16_t), 
    void (*ram_bankswitch_func)(uint16_t), 
    void (*ram_enable_func)(uint8_t), 
    uint16_t rom_start_bank,
    uint16_t rom_end_bank,
    uint16_t ram_start_bank,
    uint16_t ram_end_bank,
    uint32_t ram_size

);
uint8_t unit_test_sram_rd_wr(
    void (*memcpy_func)(uint8_t*, uint32_t, uint32_t), 
    void (*memset_func)(uint8_t*, uint32_t, uint32_t),
    uint32_t ram_size);
#endif